#include <assert.h>
#include <cmath>
#include <cstdio>
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

int main() {
    str_storage_t *storage = str_storage_t_ctor(CHUNK_SIZE);
    // str_t text = read_text_from_file(EXPRESSION_FILE_PATH);
    dot_code_t dot_code = {}; dot_code_t_ctor(&dot_code, LIST_DOT_CODE_PARS);
    dot_dir_t dot_dir = {}; dot_dir_ctor(&dot_dir, DOT_DIR_PATH, DOT_FILE_NAME, DOT_IMG_NAME);

    bin_tree_t tree = {};
    bin_tree_ctor(&tree, LOG_FILE_PATH);
    tex_dir_t tex_dir = {}; tex_dir_ctor(&tex_dir, "latex", "code.tex");
    tex_start_code(&tex_dir);
    token_t token_list[TOKEN_LIST_MAX_SZ] = {};
    str_t text = {NULL, 0};
    FILE *expression_file = fopen(EXPRESSION_FILE_PATH, "r");
    if (expression_file == NULL) {
        debug("file '%s' open failed", EXPRESSION_FILE_PATH);
        CLEAR_MEMORY(exit_mark);
    }


    while (getline(&text.str_ptr, &text.len, expression_file) != -1) { // FIXME:
        if (text.str_ptr[0] == '!') {
            fprintf(tex_dir.code_file_ptr, "%s", text.str_ptr + 1);
            continue;
        }

        defer_info_t defer_info = defer_info_t_ctor(&dot_code);
        parsing_block_t data = {0, text.str_ptr, 0, token_list, &tree, &dot_code, &storage};
        lex_scanner(&data);
        tree.root = get_G(&data);


        collect_tree_info(tree.root); write_expression_to_tex(&tex_dir, tree.root, {});

        latex_insert_phrase(&tex_dir);

        tree.root = differentiate(tree.root);
        tree.root = constant_convolution_diff_tree(tree.root);
        tree.root = neutrals_remove_diff_tree(tree.root);

        convert_subtree_to_dot(tree.root, &dot_code, &storage);

        collect_tree_info(tree.root); write_expression_to_tex(&tex_dir, tree.root, defer_info);
        FREE(text.str_ptr); // FIXME:
        text = {NULL, 0};
        bin_tree_dtor(&tree);
    }

    tex_generate_pdf(&tex_dir);
    dot_code_render(&dot_dir, &dot_code);


    sub_tree_dtor(tree.root);
    str_storage_t_dtor(storage);
    dot_code_t_dtor(&dot_code);

    return EXIT_SUCCESS;

    exit_mark:

    sub_tree_dtor(tree.root);
    str_storage_t_dtor(storage);
    dot_code_t_dtor(&dot_code);

    return EXIT_FAILURE;
}
