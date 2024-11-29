#include <stdio.h>

#include "diff_tree.h"
#include "general.h"

#include <cassert>
#include <cstdlib>
#include <stdlib.h>
#include "error_processing.h"
#include "general.h"

#include "stack_funcs.h"

// ERROR_PROCESSING \\-------------------------------------------------------------------------------->

void bin_tree_err_add(enum bin_tree_err_t *dest, enum bin_tree_err_t add) {
    *dest = (bin_tree_err_t)((unsigned long long)(*dest) | (unsigned long long) add);
}

void bin_tree_err_get_descr(const enum bin_tree_err_t err_code, char err_descr_str[]) {
    bool error = false;
    #define DESCR_(err_code, err)                 \
        {                                         \
            if (err_code & err) {                 \
                sprintf(err_descr_str, #err", "); \
                error = true;                     \
            }                                     \
        }                                         \

    DESCR_(err_code, BT_ERR_FILE_OPEN);
    DESCR_(err_code, BT_ERR_ALLOC);
    DESCR_(err_code, BT_ERR_SYSTEM);
    DESCR_(err_code, BT_ERR_STACK);
    DESCR_(err_code, BT_ERR_CYCLED);

    if (!error) {
        sprintf(err_descr_str, "ALL IS OK:)");
    }
    #undef DESCR_
}


// TREE_PROCESSING \\-------------------------------------------------------------------------------->
bool bin_tree_ctor(bin_tree_t *tree, const char log_path[]) {
    stk_err stk_last_err = STK_ERR_OK;

    tree->n_nodes = 0;

    tree->node_stack = {};
    STACK_INIT(&tree->node_stack, 0, sizeof(bin_tree_elem_t *), tree->log_file_ptr, &stk_last_err)

    if (stk_last_err != STK_ERR_OK) {
        DEBUG_BT_LIST_ERROR(BT_ERR_STACK, "node_stack ctor failed");
        CLEAR_MEMORY(exit_mark)
    }

    strcpy(tree->log_file_path, log_path);
    tree->log_file_ptr = fopen(log_path, "a");
    if (tree->log_file_ptr == NULL) {
        DEBUG_BT_LIST_ERROR(BT_ERR_FILE_OPEN, "path: {%s}", log_path)
        CLEAR_MEMORY(exit_mark)
    }

    setbuf(tree->log_file_ptr, NULL); // disable buffering

    return true;

    exit_mark:

    stack_destroy(&tree->node_stack);
    if (tree->log_file_ptr != NULL) {
        fclose(tree->log_file_ptr);
    }
    return false;
}

bool bin_tree_dtor(bin_tree_t *tree) {
    stk_err last_stack_error = STK_ERR_OK;

    for (size_t i = 0; i < tree->node_stack.size; i++) {

        bin_tree_elem_t *node_ptr = *(bin_tree_elem_t **) stack_get_elem(&tree->node_stack, i, &last_stack_error);

        if (last_stack_error != STK_ERR_OK) {
            debug("stack get elem by idx: [%lu] failed", i);
            return false;
        }

        FREE(node_ptr)
    }
    stack_destroy(&tree->node_stack);
    tree->root = NULL;
    tree->n_nodes = 0;

    return true;
}

bin_tree_elem_t *bin_tree_create_node(bin_tree_t *tree, bin_tree_elem_t *prev, const bool is_node_left_son,
    bin_tree_elem_t *left, bin_tree_elem_t *right, const bin_tree_elem_value_t data)
{
    tree->n_nodes++;

    bin_tree_elem_t *node = (bin_tree_elem_t *) calloc(1, sizeof(bin_tree_elem_t));
    if (node == NULL) {
        DEBUG_BT_LIST_ERROR(BT_ERR_ALLOC, "node alloc failed");
        return NULL;
    }

    stk_err stk_last_err = STK_ERR_OK;

    stack_push(&tree->node_stack, &node, &stk_last_err);
    if (stk_last_err != STK_ERR_OK) {
        DEBUG_BT_LIST_ERROR(BT_ERR_STACK, "stack push failed");
        return NULL;
    }

    if (prev) {
        if (is_node_left_son) {
        prev->left = node;
        } else {
            prev->right = node;
        }
        node->prev = prev;
    }

    node->left = left;
    node->right = right;
    node->data = data;
    node->is_node_left_son = is_node_left_son;

    return node;
}

void bin_tree_push_val(bin_tree_t *tree, bin_tree_elem_t *cur_node, bin_tree_elem_value_t val,
    int (*compare_func)(const bin_tree_elem_value_t node1, const bin_tree_elem_value_t node2)) {
    assert(compare_func != NULL);

    if (compare_func(val, cur_node->data)) {
        if (!cur_node->left) {
            bin_tree_create_node(tree, cur_node, true, NULL, NULL, val);
        } else {
            bin_tree_push_val(tree, cur_node->left, val, compare_func);
        }
    } else {
        if (!cur_node->right) {
            bin_tree_create_node(tree, cur_node, false, NULL, NULL, val);
        } else {
            bin_tree_push_val(tree, cur_node->right, val, compare_func);
        }
    }
}

void bin_tree_print(bin_tree_elem_t *node, void (*outp_func)(char *dest, const size_t maxn_n, const bin_tree_elem_t *node)) {
    if (!node) {
        return;
    }
    printf("(");

    if (node->left) {
        bin_tree_print(node->left, outp_func);
    }
    char node_label[NODE_LABEL_MAX_SZ] = {};
    outp_func(node_label, NODE_LABEL_MAX_SZ, node);
    printf("%s", node_label);

    if (node->right) {
        bin_tree_print(node->right, outp_func);
    }

    printf(")");
}

bool bin_tree_clear(bin_tree_t *tree) {
    stk_err last_stack_error = STK_ERR_OK;

    size_t stk_start_size = tree->node_stack.size;
    for (size_t i = 0; i < stk_start_size; i++) {


        bin_tree_elem_t *node_ptr = *(bin_tree_elem_t **) stack_get_last(&tree->node_stack, &last_stack_error);
        stack_pop(&tree->node_stack, &last_stack_error);
        FREE(node_ptr)

        if (last_stack_error != STK_ERR_OK) {
            debug("stack get elem by idx: [%lu] failed", i);
            return false;
        }


    }

    tree->root = NULL;
    tree->n_nodes = 0;
    return true;
}

void bin_tree_rec_nodes_cnt(bin_tree_elem_t *node, size_t *nodes_cnt) {
    if (!node) {
        return;
    }
    if (*nodes_cnt > MAX_NODES_CNT) {
        return;
    }

    (*nodes_cnt)++;
    if (node->left) {
        bin_tree_rec_nodes_cnt(node->left, nodes_cnt);
    }
    if (node->right) {
        bin_tree_rec_nodes_cnt(node->right, nodes_cnt);
    }
}

void bin_tree_verify(const bin_tree_t tree, bin_tree_err_t *return_err) {
    size_t nodes_cnt = 0;
    bin_tree_rec_nodes_cnt(tree.root, &nodes_cnt);
    if (nodes_cnt > MAX_NODES_CNT) {
        bin_tree_err_add(return_err, BT_ERR_CYCLED);
        debug("tree might be cycled. nodes cnt exceeds max nodes cnt value");
        return;
    }
}