#include "diff_tree.h"
#include "graphviz_funcs.h"
#include <cstdlib>

const char DOT_DIR_PATH[] = "./logs";
const char DOT_FILE_NAME[] = "graph.dot";
const char DOT_IMG_NAME[] = "gr_img.png";

const dot_node_pars_t DEFAULT_NODE_PARS = {"Mrecord", "black", "#FFEEEE", "filled"};
const dot_edge_pars_t DEFAULT_EDGE_PARS = {NULL, NULL, "#00FF00", 2};
const dot_code_pars_t LIST_DOT_CODE_PARS = {"TB"};

int main() {
    bin_tree_t tree = {};
    bin_tree_ctor(&tree, "./logs/log.html");
    dot_dir_t dot_dir = {};
    dot_dir_ctor(&dot_dir, DOT_DIR_PATH, DOT_FILE_NAME, DOT_IMG_NAME);

    dot_code_t dot_code = {};
    dot_code_t_ctor(&dot_code, LIST_DOT_CODE_PARS);

    enum node_types {
        VAR = 0,
        NUM = 1,
        OP = 2,
    };

    enum opers {
        ADD = 0,
        SUB = 1,
        MUL = 2,
        DIV = 3,
    };
    bin_tree_elem_t *node_div = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {OP, DIV});
    size_t node_div_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '/' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_add = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {OP, ADD});
    size_t node_add_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '+' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_sub = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {OP, SUB});
    size_t node_sub_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: OP | value : '-' | {<L> (L)| <R> (R)}}");
    bin_tree_elem_t *node_x = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {VAR, 1});
    bin_tree_elem_t *node_3 = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {NUM, 3});
    bin_tree_elem_t *node_100 = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {NUM, 100});
    bin_tree_elem_t *node_7 = bin_tree_create_node(&tree, NULL, false, NULL, NULL, {NUM, 7});
    size_t node_x_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: PAR | value : 'X' | {<L> (L)| <R> (R)}}");
    size_t node_3_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '3' | {<L> (L)| <R> (R)}}");
    size_t node_100_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '100' | {<L> (L)| <R> (R)}}");
    size_t node_7_idx = dot_new_node(&dot_code, DEFAULT_NODE_PARS, "{type: NUM | value : '7' | {<L> (L)| <R> (R)}}");

    node_div->left = node_add;
    node_div->right = node_sub;
    size_t edge_01 = dot_new_edge(&dot_code, node_div_idx, node_add_idx, DEFAULT_EDGE_PARS, "");
    dot_code.edge_list[edge_01].pars.start_suf = "L";
    size_t edge_02 = dot_new_edge(&dot_code, node_div_idx, node_sub_idx, DEFAULT_EDGE_PARS, "");
    dot_code.edge_list[edge_02].pars.start_suf = "R";


    node_add->left = node_x;
    size_t edge1 = dot_new_edge(&dot_code, node_add_idx, node_x_idx, DEFAULT_EDGE_PARS, "");
    dot_code.edge_list[edge1].pars.start_suf = "L";
    node_add->right = node_3;
    size_t edge2 = dot_new_edge(&dot_code, node_add_idx, node_3_idx, DEFAULT_EDGE_PARS, "");
    dot_code.edge_list[edge2].pars.start_suf = "R";


    node_sub->left = node_100;
    size_t edge3 = dot_new_edge(&dot_code, node_sub_idx, node_100_idx, DEFAULT_EDGE_PARS, "");
    dot_code.edge_list[edge3].pars.start_suf = "L";
    size_t edge4 = dot_new_edge(&dot_code, node_sub_idx, node_7_idx, DEFAULT_EDGE_PARS, "");
    node_sub->right = node_7;
    dot_code.edge_list[edge4].pars.start_suf = "R";








    dot_code_render(&dot_dir, &dot_code);

    bin_tree_dtor(&tree);

    return 0;
}