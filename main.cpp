#include <assert.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "general.h"
#include "diff_tree.h"
#include "graphviz_funcs.h"
#include "stack_funcs.h"
#include "tex_funcs.h"
#include "diff_funcs.h"
#include "string_funcs.h"
#include "diff_grammar.h"
#include "diff_DSL.h"

const char DOT_DIR_PATH[] = "./logs";
const char LOG_FILE_PATH[] = "./logs/log.html";
const char DOT_FILE_NAME[] = "graph.dot";
const char DOT_IMG_NAME[] = "gr_img.png";
const char EXPRESSION_FILE_PATH[] = "./expression.txt";

bin_tree_elem_t *differentiate(bin_tree_elem_t *node) {
    assert(node != NULL);

    bin_tree_elem_t *new_node  = NULL;

    if (node->data.type == NODE_VAR) {
        FREE(node);
        return _NUM(1);
    } else if (node->data.type == NODE_NUM) {
        FREE(node);
        return _NUM(0);
    } else if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_ADD) {
            new_node = _ADD(differentiate(node->left), differentiate(node->right));
        } else if (node->data.value.ival == OP_SUB) {
            new_node = _SUB(differentiate(node->left), differentiate(node->right));
        } else if (node->data.value.ival == OP_MUL) {

            bin_tree_elem_t *left_copy = get_tree_copy(node->left);
            bin_tree_elem_t *right_copy = get_tree_copy(node->right);
            bin_tree_elem_t *left_diff = differentiate(left_copy);
            bin_tree_elem_t *right_diff = differentiate(right_copy);

            new_node =
            _ADD(
                _MUL(left_diff, node->right),
                _MUL(right_diff, node->left)
            );
        } else if (node->data.value.ival == OP_DIV) {
            bin_tree_elem_t *left_copy = get_tree_copy(node->left);
            bin_tree_elem_t *right_copy = get_tree_copy(node->right);
            bin_tree_elem_t *left_diff = differentiate(left_copy);
            bin_tree_elem_t *right_diff = differentiate(right_copy);

            new_node = _DIV
            (
                _SUB(_MUL(left_diff, node->right), _MUL(node->left, right_diff)),
                _MUL(get_tree_copy(node->right), get_tree_copy(node->right))
            );
        }
    } else if (node->data.type == NODE_FUNC) {
        if (strcmp(node->data.value.sval, "sin") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _FUNC(node->right, "cos"));
        } else if (strcmp(node->data.value.sval, "cos") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _MUL(_NUM(-1), _FUNC(node->right, "sin")));
        } else if (strcmp(node->data.value.sval, "ln") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _DIV(_NUM(1), node->right));
        } else if (strcmp(node->data.value.sval, "sqrt") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1), _MUL(_NUM(2), get_tree_copy(node))));
        } else if (strcmp(node->data.value.sval, "tg") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1),
                _MUL(_FUNC(get_tree_copy(node->right), "cos"), _FUNC(get_tree_copy(node->right), "cos"))));
        } else if (strcmp(node->data.value.sval, "ctg") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(-1),
                _MUL(_FUNC(get_tree_copy(node->right), "sin"), _FUNC(get_tree_copy(node->right), "sin"))));
        } else if (strcmp(node->data.value.sval, "arcsin") == 0) {
            bin_tree_elem_t *radical_node = _SUB(_NUM(1), _MUL(get_tree_copy(node->right), get_tree_copy(node->right)));
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1), _FUNC(radical_node, "sqrt")));
        }
    }

    FREE(node);
    return new_node;
}

const int DEFER_LOWERBOUND = 15;
const int DEFER_UPPERBOUND = 20;
const int DEFER_DELTA = 10;
const int DEFER_DIV_COEFF = 4;

struct defer_node_t {
    char letter;
    size_t letter_idx;
    bin_tree_elem_t *ptr;
};

struct defer_info_t {
    size_t cur_tree_scale_val;
    dot_code_t *dot_code;
    defer_node_t def_list[BUFSIZ];
    size_t def_list_idx;

    char letter;
    size_t letter_idx;
};

defer_info_t defer_info_t_ctor(dot_code_t *dot_code) {
    defer_info_t defer_info = {};
    defer_info.cur_tree_scale_val = 0;
    defer_info.dot_code = dot_code;
    defer_info.def_list_idx = 0;

    defer_info.letter = 'A';
    defer_info.letter_idx = 0;
    return defer_info;
}

int def_coef_get(size_t tree_sz) {
    return (int) sqrt((double) tree_sz);
}

bool defer_check(bin_tree_elem_t *node, defer_info_t *defer_info) {
    int node_scale_val = (int) (node->sub_tree_sz + node->sup_tree_divop_cnt * DEFER_DIV_COEFF);

    if (node_scale_val < DEFER_LOWERBOUND) {
        return false;
    }

    int scale_pattern = MIN((int) DEFER_UPPERBOUND, def_coef_get(defer_info->cur_tree_scale_val));

    return (abs(node_scale_val - scale_pattern) < (int) DEFER_DELTA);
}

void write_subtree(FILE *stream, bin_tree_elem_t *node, defer_info_t *defer_info) {
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);


    if (defer_check(node, defer_info)) {

        // printf("sz: %lu, graphviz_idx = {%d}\n", node->sub_tree_sz, node->graphviz_idx);
        if (defer_info->dot_code) {
            defer_info->dot_code->node_list[node->graphviz_idx].pars.fillcolor = "#ff00ff";
        }

        defer_info->def_list[defer_info->def_list_idx].letter = defer_info->letter;
        defer_info->def_list[defer_info->def_list_idx].letter_idx = defer_info->letter_idx;
        defer_info->def_list[defer_info->def_list_idx].ptr = node;
        defer_info->def_list_idx++;


        fprintf(stream, "%c%lu", defer_info->letter, defer_info->letter_idx++);
        return;
    }

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
        fprintf(stream, "\\%s(", bufer);
        write_subtree(stream, node->right, defer_info);
        fprintf(stream, ")");
    }

    if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_DIV) {
            fprintf(stream, "\\frac{");
            if (node->left) {
                write_subtree(stream, node->left, defer_info);
            }
            fprintf(stream, "}{");
            if (node->right) {
                write_subtree(stream, node->right, defer_info);
            }
            fprintf(stream, "}");
        } else {
            bool left_const_state = (node->left->data.type == NODE_NUM);
            bool right_const_state = (node->right->data.type == NODE_NUM);

            bool left_var_state = (node->left->data.type == NODE_VAR);
            bool right_var_state = (node->right->data.type == NODE_VAR);

            if (left_const_state && right_var_state) {
                write_subtree(stream, node->left, defer_info);
                write_subtree(stream, node->right, defer_info);
            } else if (right_const_state && left_var_state) {
                write_subtree(stream, node->right, defer_info);
                write_subtree(stream, node->left, defer_info);
            } else {
                if (node->left) {
                    write_subtree(stream, node->left, defer_info);
                }

                if (node->data.value.ival == OP_MUL) {
                    fprintf(stream, " \\cdot ");
                } else {
                    fprintf(stream, " %s ", bufer);
                }

                if (node->right) {
                    write_subtree(stream, node->right, defer_info);
                }
            }
        }

        return;
    }
}

bool make_tex_of_subtree(tex_dir_t *tex_dir, bin_tree_elem_t *root, defer_info_t defer_info) {
    assert(tex_dir);
    assert(root);

    defer_info.cur_tree_scale_val = root->sub_tree_sz + root->sup_tree_divop_cnt * DEFER_DIV_COEFF;
    defer_info.def_list_idx = 0;

    fprintf(tex_dir->code_file_ptr, "$$");
    write_subtree(tex_dir->code_file_ptr, root, &defer_info);
    fprintf(tex_dir->code_file_ptr, "$$ \\\\  \\\\ \n");

    size_t prev_def_idx = defer_info.def_list_idx;
    defer_info.def_list_idx = 0;

    for (size_t i = 0; i < prev_def_idx; i++) {
        defer_node_t d_node = defer_info.def_list[i];
        defer_info.cur_tree_scale_val = d_node.ptr->sub_tree_sz + d_node.ptr->sup_tree_divop_cnt * DEFER_DIV_COEFF;

        fprintf(tex_dir->code_file_ptr, "%c%lu = ", d_node.letter, d_node.letter_idx);

        fprintf(tex_dir->code_file_ptr, "$");
        write_subtree(tex_dir->code_file_ptr, d_node.ptr, &defer_info);
        fprintf(tex_dir->code_file_ptr, "$ \\\\ \n");
    }

    return true;
}

void collect_tree_info(bin_tree_elem_t *root) {
    assert(root);

    root->sub_tree_sz = 1;
    root->sup_tree_divop_cnt = 0;

    if (root->data.type == NODE_OP && root->data.value.ival == OP_DIV) {
        root->sup_tree_divop_cnt++;
    }

    if (root->left) {
        collect_tree_info(root->left);
        root->sub_tree_sz += root->left->sub_tree_sz;
        root->sup_tree_divop_cnt += root->left->sup_tree_divop_cnt;
    }
    if (root->right) {
        collect_tree_info(root->right);
        root->sub_tree_sz += root->right->sub_tree_sz;
        root->sup_tree_divop_cnt += root->right->sup_tree_divop_cnt;
    }
}

void write_expression_to_tex(tex_dir_t *tex_dir, bin_tree_elem_t *root, defer_info_t defer_info) {
    assert(root);
    // fprintf(tex_dir->code_file_ptr, "%c%lu = \n", letter, letter_idx++);
    make_tex_of_subtree(tex_dir, root, defer_info);

    // if (root->left) {
    //     write_expression_to_tex(tex_dir, root->left);
    // }
    // if (root->right) {
    //     write_expression_to_tex(tex_dir, root->right);
    // }

    // cnt = 0;
    // letter = 'A';

    // for (size_t i = 0; i < deferred_subtrees_idx; i++) {
    //     if (!deferred_subtrees[i].ptr) {
    //         debug("deferred_subtrees[%lu]->ptr = nullptr", i);
    //     }
    //     assert(deferred_subtrees[i].ptr);
    //     fprintf(stream, "%c = \n", deferred_subtrees[i].letter);
    //     next_tex_rec_write_node(stream, deferred_subtrees[i].ptr);
    //     cnt = 0;
    // }
}

int main() {
    str_storage_t *storage = str_storage_t_ctor(CHUNK_SIZE);
    str_t text = read_text_from_file(EXPRESSION_FILE_PATH);
    dot_code_t dot_code = {}; dot_code_t_ctor(&dot_code, LIST_DOT_CODE_PARS);
    dot_dir_t dot_dir = {}; dot_dir_ctor(&dot_dir, DOT_DIR_PATH, DOT_FILE_NAME, DOT_IMG_NAME);

    bin_tree_t tree = {};
    bin_tree_ctor(&tree, LOG_FILE_PATH);
    tex_dir_t tex_dir = {}; tex_dir_ctor(&tex_dir, "latex", "code.tex");
    tex_start_code(&tex_dir);
    token_t token_list[TOKEN_LIST_MAX_SZ] = {};

    parsing_block_t data = {0, text.str_ptr, 0, token_list, &tree, &dot_code, &storage};
    lex_scanner(&data);

    // draw_parsing_text(&data);

    tree.root = get_G(&data);





    tree.root = differentiate(tree.root);
    convert_subtree_to_dot(tree.root, &dot_code, &storage);



    // tree.root = constant_convolution_diff_tree(tree.root);
    // tree.root = neutrals_remove_diff_tree(tree.root);






    // printf("infix: '"); write_infix(tree.root); printf("'\n");

    defer_info_t defer_info = defer_info_t_ctor(&dot_code);
    collect_tree_info(tree.root); write_expression_to_tex(&tex_dir, tree.root, defer_info);


    tex_generate_pdf(&tex_dir);
    dot_code_render(&dot_dir, &dot_code);
    bin_tree_dtor(&tree);
    sub_tree_dtor(tree.root);
    FREE(text.str_ptr)
    str_storage_t_dtor(storage);
    dot_code_t_dtor(&dot_code);

    return 0;
}
