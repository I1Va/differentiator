#include <assert.h>
#include <cstdlib>
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





    tree.root = differentiate(tree.root);

    // convert_subtree_to_dot(tree.root, &dot_code, &storage);
    tree.root = constant_convolution_diff_tree(tree.root);
    tree.root = neutrals_remove_diff_tree(tree.root);
    convert_subtree_to_dot(tree.root, &dot_code, &storage);







    printf("infix: '"); write_infix(tree.root); printf("'\n");

    make_tex_of_subtree("./latex", "code.tex", tree.root);




    dot_code_render(&dot_dir, &dot_code);

    sub_tree_dtor(tree.root);
    FREE(text.str_ptr)
    str_storage_t_dtor(storage);
    // dot_dir_dtor(&dot_dir);
    dot_code_t_dtor(&dot_code);

    return 0;
}
