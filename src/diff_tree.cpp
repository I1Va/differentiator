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
    assert(tree != NULL);

    strcpy(tree->log_file_path, log_path);
    tree->log_file_ptr = fopen(log_path, "a");
    if (tree->log_file_ptr == NULL) {
        DEBUG_BT_LIST_ERROR(BT_ERR_FILE_OPEN, "path: {%s}", log_path)
        CLEAR_MEMORY(exit_mark)
    }

    setbuf(tree->log_file_ptr, NULL); // disable buffering

    return true;

    exit_mark:
    if (tree->log_file_ptr != NULL) {
        fclose(tree->log_file_ptr);
    }
    return false;
}

bin_tree_elem_t *bin_tree_create_node(bin_tree_elem_t *left, bin_tree_elem_t *right, const bin_tree_elem_value_t data) {

    bin_tree_elem_t *node = (bin_tree_elem_t *) calloc(1, sizeof(bin_tree_elem_t));
    if (node == NULL) {
        DEBUG_BT_LIST_ERROR(BT_ERR_ALLOC, "node alloc failed");
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->data = data;

    if (left) {
        left->prev = node;
        left->is_node_left_son = true;
    }
    if (right) {
        right->prev = node;
        right->is_node_left_son = false;
    }

    return node;
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

void bin_tree_dtor(bin_tree_elem_t *root) {
    if (!root) {
        return;
    }

    if (root->left) {
        bin_tree_dtor(root->left);
    }
    if (root->right) {
        bin_tree_dtor(root->right);
    }
    FREE(root);
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