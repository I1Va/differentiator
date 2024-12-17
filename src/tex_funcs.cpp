#include <assert.h>
#include <cstdio>

#include "diff_funcs.h"
#include "diff_tree.h"
#include "general.h"
#include "tex_funcs.h"

const char *MATH_PHRASES[] =
{
    "Every Soviet schoolchild understands",
    "Obviously",
    "It's not hard to notice",
};

const size_t PHRASES_CNT = sizeof(MATH_PHRASES) / sizeof(const char *);

bool tex_dir_ctor(tex_dir_t *tex_dir, const char dir[], const char name[]) {
    assert(dir != NULL);
    assert(name != NULL);
    assert(tex_dir != NULL);

    strncpy(tex_dir->dir, dir, MEDIUM_BUFER_SZ);
    strncpy(tex_dir->name, name, MEDIUM_BUFER_SZ);
    snprintf(tex_dir->code_path, MEDIUM_BUFER_SZ, "%s/%s", dir, name);

    tex_dir->code_file_ptr = fopen(tex_dir->code_path, "w");
    if (!tex_dir->code_file_ptr) {
        debug("can't open : '%s'", tex_dir->code_path);
        return false;
    }

    return true;
}

void tex_dir_dtor(tex_dir_t *tex_dir) {
    if (tex_dir && tex_dir->code_file_ptr) {
        fclose(tex_dir->code_file_ptr);
    }
}

void tex_start_code(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "\\documentclass[12pt]{article}\n");
    fprintf(tex_dir->code_file_ptr, "\\usepackage{geometry}\n");
    fprintf(tex_dir->code_file_ptr, "\\usepackage{pgfplots}\n");
    fprintf(tex_dir->code_file_ptr, "\\pgfplotsset{compat=1.9}\n");


    fprintf(tex_dir->code_file_ptr, \
    "\\geometry{ \n"
    "a4paper, \n"
    "top=25mm, \n"
    "right=15mm, \n"
    "bottom=25mm, \n"
    "left=30mm} \n"
    );

    fprintf(tex_dir->code_file_ptr, "\\begin{document}\n");
}

void tex_close_code(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "\\end{document}\n");
    fclose(tex_dir->code_file_ptr);
}

void tex_generate_pdf(tex_dir_t *tex_dir) {
    tex_close_code(tex_dir);

    char bufer[MEDIUM_BUFER_SZ] = {};
    snprintf(bufer, MEDIUM_BUFER_SZ, "cd %s && pdflatex %s", tex_dir->dir, tex_dir->name);
    printf("bufer : '%s'\n", bufer);
    system(bufer);
}

void latex_insert_phrase(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "%s \\\\ \n", MATH_PHRASES[((size_t) rand()) % PHRASES_CNT]);
}

void write_subtree(FILE *stream, bin_tree_elem_t *node, defer_info_t *defer_info, tex_info_t tex_info) {
    assert(node);
    assert(stream);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);


    // printf("height: %lu, root_height: %lf: ", node->subtree_info.height, defer_info->tree_scale_val);
    if (defer_check(node, defer_info)) {
        // printf("sz: %lu, graphviz_idx = {%d}\n", node->sub_tree_sz, node->graphviz_idx);
        if (defer_info->dot_code) {
            defer_info->dot_code->node_list[node->graphviz_idx].pars.fillcolor = "#ff00ff";
        }

        defer_info->def_list[defer_info->def_list_idx].letter = defer_info->letter;
        defer_info->def_list[defer_info->def_list_idx].letter_idx = defer_info->letter_idx;
        defer_info->def_list[defer_info->def_list_idx].ptr = node;
        defer_info->def_list_idx++;


        fprintf(stream, "%c%d", defer_info->letter, defer_info->letter_idx++);
        return;
    }
    // printf("NO\n");

    if (node->data.type == NODE_VAR) {
        fprintf(stream, "%s", bufer);
        return;
    }

    if (node->data.type == NODE_NUM) {
        if (node->data.value.lval < 0) {
            fprintf(stream, "(%s)", bufer);
        } else {
            fprintf(stream, "%s", bufer);
        }
        return;
    }

    if (node->data.type == NODE_FUNC) {
        // fprintf(stream, "\\%s(", bufer);
        fprintf(stream, "%s(", bufer);
        write_subtree(stream, node->right, defer_info, tex_info);
        fprintf(stream, ")");
    }

    if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_DIV) {
            if (tex_info.frac_state) {
                fprintf(stream, "\\frac{");
                if (node->left) {
                    write_subtree(stream, node->left, defer_info, tex_info);
                }
                fprintf(stream, "}{");
                if (node->right) {
                    write_subtree(stream, node->right, defer_info, tex_info);
                }
                fprintf(stream, "}");
            } else {
                fprintf(stream, "(");
                if (node->left) {
                    write_subtree(stream, node->left, defer_info, tex_info);
                }
                fprintf(stream, ")/(");
                if (node->right) {
                    write_subtree(stream, node->right, defer_info, tex_info);
                }
                fprintf(stream, ")");
            }
        } else if (node->data.value.ival == OP_POW) {
            fprintf(stream, "(");
            if (node->left) {
                write_subtree(stream, node->left, defer_info, tex_info);
            }
            fprintf(stream, ")^{");
            if (node->right) {
                write_subtree(stream, node->right, defer_info, tex_info);
            }
            fprintf(stream, "}");
        } else {
            bool left_const_state = (node->left->data.type == NODE_NUM);
            bool right_const_state = (node->right->data.type == NODE_NUM);

            bool left_var_state = (node->left->data.type == NODE_VAR);
            bool right_var_state = (node->right->data.type == NODE_VAR);

            if (left_const_state && right_var_state) {
                write_subtree(stream, node->left, defer_info, tex_info);
                write_subtree(stream, node->right, defer_info, tex_info);
            } else if (right_const_state && left_var_state) {
                write_subtree(stream, node->right, defer_info, tex_info);
                write_subtree(stream, node->left, defer_info, tex_info);
            } else {
                if (node->left) {
                    write_subtree(stream, node->left, defer_info, tex_info);
                }

                if (node->data.value.ival == OP_MUL) {
                    if (tex_info.mul_cdot_state) {
                        fprintf(stream, " \\cdot ");
                    } else {
                        fprintf(stream, "*");
                    }
                } else {
                    fprintf(stream, " %s ", bufer);
                }

                if (node->right) {
                    write_subtree(stream, node->right, defer_info, tex_info);
                }
            }
        }

        return;
    }
}

bool make_tex_of_subtree(tex_dir_t *tex_dir, bin_tree_elem_t *root, defer_info_t defer_info, tex_info_t tex_info) {
    assert(tex_dir);
    assert(root);

    defer_info.tree_scale_val = calc_subtree_scale_val(root->subtree_info);
    defer_info.def_list_idx = 0;

    fprintf(tex_dir->code_file_ptr, "$$");
    write_subtree(tex_dir->code_file_ptr, root, &defer_info, tex_info);
    fprintf(tex_dir->code_file_ptr, "$$ \n");

    size_t prev_def_idx = defer_info.def_list_idx;
    defer_info.def_list_idx = 0;

    for (size_t i = 0; i < prev_def_idx; i++) {
        defer_node_t d_node = defer_info.def_list[i];
        defer_info.tree_scale_val = calc_subtree_scale_val(d_node.ptr->subtree_info);

        fprintf(tex_dir->code_file_ptr, "%c%d = ", d_node.letter, d_node.letter_idx);

        fprintf(tex_dir->code_file_ptr, "$");
        write_subtree(tex_dir->code_file_ptr, d_node.ptr, &defer_info, tex_info);
        fprintf(tex_dir->code_file_ptr, "$ \\\\ \n");
    }

    return true;
}
