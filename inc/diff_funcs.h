#ifndef DIFF_FUNCS_H
#define DIFF_FUNCS_H

#include "diff_tree.h"
#include <string.h>
#include "graphviz_funcs.h"
#include "string_funcs.h"

const dot_node_pars_t DEFAULT_NODE_PARS = {"Mrecord", "black", "#FFEEEE", "filled"};
const dot_edge_pars_t DEFAULT_EDGE_PARS = {NULL, NULL, "#00FF00", 2};
const dot_code_pars_t LIST_DOT_CODE_PARS = {"TB"};

const char VALID_OPERATIONS[] = "+/*-";
const size_t VALID_OPERATIONS_CNT = strlen(VALID_OPERATIONS);

const size_t MINI_BUFER_SZ = 16;
const size_t MEDIUM_BUFER_SZ = 128;
const size_t CHUNK_SIZE = 2048;
const size_t MAX_NODE_WRAP_SZ = 64;

const double DEFER_LOWERBOUND = 5;
const double DEFER_LOW_DELTA = 0.5;
const double DEFER_HIGH_DELTA = 0.8;

struct defer_node_t {
    char letter;
    int letter_idx;
    bin_tree_elem_t *ptr;
};

struct defer_info_t {
    char letter;
    bool defer_state;
    double defer_lowerbound;
    double low_delta;
    double high_delta;
    dot_code_t *dot_code;

    double tree_scale_val;
    defer_node_t def_list[BUFSIZ];
    size_t def_list_idx;

    int letter_idx;
};

enum node_types {
    NODE_EMPTY = 0,
    NODE_VAR = 1,
    NODE_NUM = 2,
    NODE_OP = 3,
    NODE_FUNC = 4,
};


enum opers {
    OP_ADD = 0,
    OP_DIV = 1,
    OP_MUL = 2,
    OP_SUB = 3,
    OP_POW = 4,
};

void get_node_type(enum node_types *type, long double *value, char *name);
void get_node_string(char *bufer, bin_tree_elem_t *node);
size_t seg_char_cnt(char *left, char *right, char c);
void diff_infix_print(FILE *stream, bin_tree_elem_t *node);
void fprintf_seg(FILE *stream, char *left, char *right);
char *get_end_bracket_ptr(char *start, char *end);
char *get_string_untill_bracket(char *left, char *right, char *bufer);
bin_tree_elem_t *diff_load_infix_expr(bin_tree_t *tree, bin_tree_elem_t *prev, bool prev_left, char *left, char *right, str_storage_t **storage);
int convert_subtree_to_dot(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage);
bool convert_tree_to_dot(bin_tree_t *tree, dot_code_t *dot_code, str_storage_t **storage);
void node_dump(FILE *log_file, bin_tree_elem_t *node);
bin_tree_elem_t *constant_convolution_diff_tree(bin_tree_elem_t *node);
void write_infix(bin_tree_elem_t *node);
bin_tree_elem_t *neutrals_remove_diff_tree(bin_tree_elem_t *node);
bin_tree_elem_t *roll_up_null_mult(bin_tree_elem_t *node);
bin_tree_elem_t *constant_convolution_diff_tree(bin_tree_elem_t *node);
defer_info_t defer_info_t_ctor(dot_code_t *dot_code);
double def_coef_get(double scale_val);
double calc_subtree_scale_val(subtree_info_t info);
bool defer_check(bin_tree_elem_t *node, defer_info_t *defer_info);
subtree_info_t get_node_info(bin_tree_elem_t *root);
void merge_subtrees_info(subtree_info_t *dest, subtree_info_t src);
void collect_tree_info(bin_tree_elem_t *root);

#endif // DIFF_FUNCS_H