#include <assert.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "diff_tree.h"
#include "general.h"
#include "graphviz_funcs.h"
#include "stack_funcs.h"
#include "diff_funcs.h"
#include "string_funcs.h"
#include "diff_grammar.h"
#include "diff_DSL.h"

const char DOT_DIR_PATH[] = "./logs";
const char LOG_FILE_PATH[] = "./logs/log.html";
const char DOT_FILE_NAME[] = "graph.dot";
const char DOT_IMG_NAME[] = "gr_img.png";
const char EXPRESSION_FILE_PATH[] = "./expression.txt";

void test2_read_tree_from_file() {
    bin_tree_t tree = {};
    bin_tree_ctor(&tree, LOG_FILE_PATH);
    dot_dir_t dot_dir = {};
    dot_dir_ctor(&dot_dir, DOT_DIR_PATH, DOT_FILE_NAME, DOT_IMG_NAME);
    dot_code_t dot_code = {};
    dot_code_t_ctor(&dot_code, LIST_DOT_CODE_PARS);
    str_storage_t *storage = str_storage_t_ctor(CHUNK_SIZE);

    // test1_manualy_create_tree(&tree, &dot_code);


    // diff_infix_print(stdout, tree.root);





    str_t text = read_text_from_file(EXPRESSION_FILE_PATH);
    remove_chars_from_text(&text, " \n");
    printf("text : '%s'\n", text.str_ptr);
    tree.root = diff_load_infix_expr(&tree, NULL, false, text.str_ptr, text.str_ptr + text.len, &storage);
    convert_subtree_to_dot(tree.root, &dot_code, &storage);



    dot_code_render(&dot_dir, &dot_code);

    bin_tree_dtor(&tree);
    FREE(text.str_ptr)
    str_storage_t_dtor(storage);
}

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
        }
    }

    FREE(node);
    return new_node;
}

bin_tree_elem_t *neutrals_remove_diff_tree(bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->data.type == NODE_NUM) {
        return node;
    }
    if (node->data.type == NODE_VAR) {
        return node;
    }

    if (node->data.type == NODE_FUNC) {
        node->right = neutrals_remove_diff_tree(node->right);
        return node;
    }

    if (node->data.type == NODE_OP) {
        assert(node->left != NULL);
        assert(node->right != NULL);

        bin_tree_elem_t *left = neutrals_remove_diff_tree(node->left);
        bin_tree_elem_t *right = neutrals_remove_diff_tree(node->right);

        node->left = left;
        node->right = right;

        assert(left != NULL);
        assert(right != NULL);

        bin_tree_elem_t *new_node = NULL;

        if (node->data.value.ival == OP_MUL) {
            if (left->data.type == NODE_NUM && left->data.value.lval == 1) {
                new_node = right;
                FREE(left);
                FREE(node)
            } else if (right->data.type == NODE_NUM && right->data.value.lval == 1) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_ADD) {
            if (left->data.type == NODE_NUM && left->data.value.lval == 0) {
                new_node = right;
                FREE(left);
                FREE(node)
            } else if (right->data.type == NODE_NUM && right->data.value.lval == 0) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_DIV) {
            if (right->data.type == NODE_NUM && right->data.value.lval == 1) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_SUB) {
            if (right->data.type == NODE_NUM && right->data.value.lval == 0) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

    }

    debug("unknown node_type : {%d}", node->data.type);
    return NULL;
}

bin_tree_elem_t *constant_state_convolution_diff_tree(bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->data.type == NODE_NUM) {
        node->constant_state = true;
        return node;
    }
    if (node->data.type == NODE_VAR) {
        return node;
    }

    if (node->data.type == NODE_FUNC) {
        node->right = constant_state_convolution_diff_tree(node->right);
        return node;
    }

    if (node->data.type == NODE_OP) {
        assert(node->left != NULL);
        assert(node->right != NULL);

        bin_tree_elem_t *left = constant_state_convolution_diff_tree(node->left);
        bin_tree_elem_t *right = constant_state_convolution_diff_tree(node->right);

        node->left = left;
        node->right = right;

        bin_tree_elem_t *new_node = NULL;

        if (left->constant_state && right->constant_state) {
            assert(left->data.type == NODE_NUM);
            assert(right->data.type == NODE_NUM);

            if (node->data.value.ival == OP_MUL) {
                new_node = _NUM(left->data.value.lval * right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_ADD) {
                new_node = _NUM(left->data.value.lval + right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_DIV) {
                assert(right->data.value.lval != 0);

                new_node = _NUM(left->data.value.lval / right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_SUB) {
                new_node = _NUM(left->data.value.lval - right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else {
                debug("unknown op {%d}", node->data.value.ival);
            }
            new_node->constant_state = true;
            return new_node;
        }

        return node;
    }

    debug("unknown node_type : {%d}", node->data.type);
    return NULL;
}

void make_latex_code(FILE *stream, bin_tree_elem_t *node) {
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);

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

    if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_DIV) {
            fprintf(stream, "(");
            if (node->left) {
                make_latex_code(stream, node->left);
            }
            fprintf(stream, ")");

            fprintf(stream, "%s", bufer);

            fprintf(stream, "(");
            if (node->right) {
                make_latex_code(stream, node->right);
            }
            fprintf(stream, ")");
        } else {
            if (node->left) {
                make_latex_code(stream, node->left);
            }

            fprintf(stream, "%s", bufer);

            if (node->right) {
                make_latex_code(stream, node->right);
            }
        }

        return;
    }

    if (node->data.type == NODE_FUNC) {
        fprintf(stream, "%s(", bufer);
        make_latex_code(stream, node->right);
        fprintf(stream, ")");
    }
}

int main() {
    str_storage_t *storage = str_storage_t_ctor(CHUNK_SIZE);
    str_t text = read_text_from_file(EXPRESSION_FILE_PATH);
    dot_dir_t dot_dir = {}; dot_dir_ctor(&dot_dir, DOT_DIR_PATH, DOT_FILE_NAME, DOT_IMG_NAME);
    dot_code_t dot_code = {}; dot_code_t_ctor(&dot_code, LIST_DOT_CODE_PARS);
    bin_tree_t tree = {};
    bin_tree_ctor(&tree, LOG_FILE_PATH);

    printf("text : '%s'\n", text.str_ptr);


    token_t token_list[TOKEN_LIST_MAX_SZ] = {};

    parsing_block_t data = {0, text.str_ptr, 0, token_list, &tree, &dot_code, &storage};
    lex_scanner(&data);

    // draw_parsing_text(&data);

    tree.root = get_G(&data);
    convert_subtree_to_dot(tree.root, &dot_code, &storage);



    // tree.root = differentiate(tree.root);
    tree.root = constant_state_convolution_diff_tree(tree.root);
    tree.root = neutrals_remove_diff_tree(tree.root);

    convert_subtree_to_dot(tree.root, &dot_code, &storage);



    printf("infix: '"); write_infix(tree.root); printf("'\n");


    FILE *latex_code_file = fopen("./code.tex", "w");
    make_latex_code(latex_code_file, tree.root);
    fclose(latex_code_file);







    dot_code_render(&dot_dir, &dot_code);

    sub_tree_dtor(tree.root);
    FREE(text.str_ptr)
    str_storage_t_dtor(storage);
    // dot_dir_dtor(&dot_dir);
    dot_code_t_dtor(&dot_code);

    return 0;
}
