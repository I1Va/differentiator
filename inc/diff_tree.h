#ifndef DIFF_TREE_H
#define DIFF_TREE_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "stack_funcs.h"


// ERROR_PROCESSING \\-------------------------------------------------------------------------------->

enum bin_tree_err_t {
    BT_ERR_OK                       = 0ull,
    BT_ERR_FILE_OPEN                = 1ull << 0,
    BT_ERR_ALLOC                    = 1ull << 1,
    BT_ERR_SYSTEM                   = 1ull << 2,
    BT_ERR_STACK                    = 1ull << 3,
    BT_ERR_CYCLED                   = 1ull << 4,
};


const size_t ERR_DESCR_MAX_SIZE = 128;
const size_t MAX_NODES_CNT = 1ull << 17;

void bin_tree_err_get_descr(enum bin_tree_err_t err_code, char err_descr_str[]);
void bin_tree_err_add(enum bin_tree_err_t *dest, enum bin_tree_err_t add);

#ifdef _DEBUG
    #define DEBUG_BT_LIST_ERROR(err_code, str_, ...) {                                                      \
        char BT_list_err_desr_str[ERR_DESCR_MAX_SIZE];                                                    \
        bin_tree_err_get_descr(err_code, BT_list_err_desr_str); \
        printf("DESCR: '%s'\n", BT_list_err_desr_str);                                                      \
        fprintf_red(stderr, "{%s} [%s: %d]: err_descr: {%s}, message: {" str_ "}\n",              \
             __FILE_NAME__, __PRETTY_FUNCTION__, __LINE__, BT_list_err_desr_str, ##__VA_ARGS__);                    \
        fprintf(stderr, WHT); \
    }

    #define ON_DEBUG(...) __VA_ARGS__
#else
    #define DEBUG_BT_LIST_ERROR(err_code, str_, ...) ;
#endif // _DEBUG


// TREE_PROCESSING \\-------------------------------------------------------------------------------->

const size_t MAX_NODE_STRING_SZ = 128;
const size_t MAX_LOG_FILE_PATH_SZ = 128;
const size_t NODE_LABEL_MAX_SZ = 128;

union multi_val_t {
    int          ival;
    long long    lval;
    long double  fval;
    char        *sval;
};

struct bin_tree_elem_value_t {
    int type;
    multi_val_t value;
};

struct bin_tree_elem_t {
    bin_tree_elem_t *prev;
    bool is_node_left_son;

    bin_tree_elem_t *left;
    bin_tree_elem_t *right;

    bin_tree_elem_value_t data;
};

typedef bin_tree_elem_t* stack_elem_t;


struct bin_tree_t {
    bin_tree_elem_t *root;

    size_t n_nodes;


    FILE *log_file_ptr;
    char log_file_path[MAX_LOG_FILE_PATH_SZ];
    stack_t node_stack;
};


void bin_tree_push_val(bin_tree_t *tree, bin_tree_elem_t *cur_node, bin_tree_elem_value_t val,
    int (*compare_func)(const bin_tree_elem_value_t node1, const bin_tree_elem_value_t node2));
bool bin_tree_ctor(bin_tree_t *tree, const char log_path[]);
bool bin_tree_dtor(bin_tree_t *tree);
bin_tree_elem_t *bin_tree_create_node(bin_tree_t *tree, bin_tree_elem_t *prev, const bool prev_left,
    bin_tree_elem_t *left, bin_tree_elem_t *right, const bin_tree_elem_value_t data);
void bin_tree_print(bin_tree_elem_t *node, void (*outp_func)(char *dest, const size_t maxn_n, const bin_tree_elem_t *node));
bool bin_tree_clear(bin_tree_t *tree);

int node_t_cmp(const bin_tree_elem_value_t node1, const bin_tree_elem_value_t node2);

bool bin_tree_generate_graph_img(bin_tree_t *tree, char short_img_path[]);
void bin_tree_log_dump(bin_tree_t *tree, const char file_name[], const char func_name[], const int line_idx);
void bin_tree_rec_nodes_cnt(bin_tree_elem_t *node, size_t *nodes_cnt);
void bin_tree_verify(const bin_tree_t tree, bin_tree_err_t *return_err);

#endif // DIFF_TREE_H