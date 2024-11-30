#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "diff_tree.h"
#include "general.h"
#include "graphviz_funcs.h"
#include "stack_funcs.h"
#include "diff_funcs.h"
#include "string_funcs.h"
#include "diff_grammar.h"

const char DOT_DIR_PATH[] = "./logs";
const char LOG_FILE_PATH[] = "./logs/log.html";
const char DOT_FILE_NAME[] = "graph.dot";
const char DOT_IMG_NAME[] = "gr_img.png";
const char EXPRESSION_FILE_PATH[] = "./expression.txt";

void test1_manualy_create_tree(bin_tree_t *tree, dot_code_t *dot_code) {

    bin_tree_elem_t *node_div = bin_tree_create_node(tree, NULL, false, NULL, NULL, {OP, DIV});
    tree->root = node_div;

    size_t node_div_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '/' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_add = bin_tree_create_node(tree, NULL, false, NULL, NULL, {OP, ADD});
    size_t node_add_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '+' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_sub = bin_tree_create_node(tree, NULL, false, NULL, NULL, {OP, SUB});
    size_t node_sub_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '-' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_x = bin_tree_create_node(tree, NULL, false, NULL, NULL, {VAR, 1});
    bin_tree_elem_t *node_3 = bin_tree_create_node(tree, NULL, false, NULL, NULL, {NUM, 3});
    bin_tree_elem_t *node_100 = bin_tree_create_node(tree, NULL, false, NULL, NULL, {NUM, 100});
    bin_tree_elem_t *node_7 = bin_tree_create_node(tree, NULL, false, NULL, NULL, {NUM, 7});
    size_t node_x_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: PAR | value : 'X' | {<L> (L)| <R> (R)}}");
    size_t node_3_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '3' | {<L> (L)| <R> (R)}}");
    size_t node_100_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '100' | {<L> (L)| <R> (R)}}");
    size_t node_7_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '7' | {<L> (L)| <R> (R)}}");

    node_div->left = node_add;
    node_div->right = node_sub;
    size_t edge_01 = dot_new_edge(dot_code, node_div_idx, node_add_idx, DEFAULT_EDGE_PARS, "");
    dot_code->edge_list[edge_01].pars.start_suf = "L";
    size_t edge_02 = dot_new_edge(dot_code, node_div_idx, node_sub_idx, DEFAULT_EDGE_PARS, "");
    dot_code->edge_list[edge_02].pars.start_suf = "R";


    node_add->left = node_x;
    size_t edge1 = dot_new_edge(dot_code, node_add_idx, node_x_idx, DEFAULT_EDGE_PARS, "");
    dot_code->edge_list[edge1].pars.start_suf = "L";
    node_add->right = node_3;
    size_t edge2 = dot_new_edge(dot_code, node_add_idx, node_3_idx, DEFAULT_EDGE_PARS, "");
    dot_code->edge_list[edge2].pars.start_suf = "R";


    node_sub->left = node_100;
    size_t edge3 = dot_new_edge(dot_code, node_sub_idx, node_100_idx, DEFAULT_EDGE_PARS, "");
    dot_code->edge_list[edge3].pars.start_suf = "L";
    size_t edge4 = dot_new_edge(dot_code, node_sub_idx, node_7_idx, DEFAULT_EDGE_PARS, "");
    node_sub->right = node_7;
    dot_code->edge_list[edge4].pars.start_suf = "R";
}

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
    convert_tree_to_dot(tree.root, &dot_code, &storage);



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
    convert_tree_to_dot(tree.root, &dot_code, &storage);

    // differentiate(&tree, tree.root);
    // printf("G: %d\n", get_G(&data));

    dot_code_render(&dot_dir, &dot_code);

    FREE(text.str_ptr)
    str_storage_t_dtor(storage);
    // dot_dir_dtor(&dot_dir);
    dot_code_t_dtor(&dot_code);

    return 0;
}