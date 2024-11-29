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


enum node_types {
    VAR = 0,
    NUM = 1,
    OP = 2,
    FUNC = 3,
};



enum opers {
    ADD = 0,
    DIV = 1,
    MUL = 2,
    SUB = 3,
};

void get_node_type(enum node_types *type, long double *value, char *name);

void get_node_string(char *bufer, bin_tree_elem_t *node);

size_t seg_char_cnt(char *left, char *right, char c);

void diff_infix_print(FILE *stream, bin_tree_elem_t *node);

void fprintf_seg(FILE *stream, char *left, char *right);

char *get_end_bracket_ptr(char *start, char *end);

char *get_string_untill_bracket(char *left, char *right, char *bufer);

bin_tree_elem_t *diff_load_infix_expr(bin_tree_t *tree, bin_tree_elem_t *prev, bool prev_left, char *left, char *right, str_storage_t **storage);

int convert_tree_to_dot(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage);

#endif // DIFF_FUNCS_H