#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "diff_tree.h"
#include "diff_funcs.h"
#include "string_funcs.h"
#include "general.h"
#include "diff_DSL.h"

const char VAR_COLOR[] = "#d56050";
const char NUM_COLOR[] = "#ddd660";
const char FUNC_COLOR[] = "#883060";
const char OP_COLOR[] = "#04859D";

void get_node_type(enum node_types *type, long double *value, char *name) {
    if (sscanf(name, "%Lf", value)) {
        *type = NODE_NUM;
        return;
    }

    if (strlen(name) == 1) {
        *type = NODE_OP;

        for (size_t i = 0; i < VALID_OPERATIONS_CNT; i++) {
            if (*name == VALID_OPERATIONS[i]) {
                *value = i;
                return;
            }
        }
    }

    *type = NODE_VAR;
    *value = 0;
}

void get_node_string(char *bufer, bin_tree_elem_t *node) {
    if (node == NULL) {
        snprintf(bufer, BUFSIZ, "NULL");
        return;
    }

    if (node->data.type == NODE_OP) {
        char res = '\0';

        switch (node->data.value.ival) {
            case OP_ADD: res = '+'; break;
            case OP_DIV: res = '/'; break;
            case OP_MUL: res = '*'; break;
            case OP_SUB: res = '-'; break;
            default: res = '?'; break;
        }

        snprintf(bufer, BUFSIZ, "%c", res);
    } else if (node->data.type == NODE_NUM) {
        snprintf(bufer, BUFSIZ, "%Ld", node->data.value.lval);
    } else if (node->data.type == NODE_VAR) {
        snprintf(bufer, BUFSIZ, "x");
    } else if (node->data.type == NODE_FUNC) {
        snprintf(bufer, BUFSIZ, "%s", node->data.value.sval);
    }

    else {
        snprintf(bufer, BUFSIZ, "?");
    }
}

size_t seg_char_cnt(char *left, char *right, char c) {
    size_t cnt = 0;

    while (left <= right) {
        cnt += (*left++ == c);
    }

    return cnt;
}

void diff_infix_print(FILE *stream, bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->left) {
        fprintf(stream, "(");
        diff_infix_print(stream, node->left);
        fprintf(stream, ")");
    }

    char bufer[MINI_BUFER_SZ] = {};
    get_node_string(bufer, node);
    fprintf(stream, "%s", bufer);

    if (node->right) {
        fprintf(stream, "(");
        diff_infix_print(stream, node->right);
        fprintf(stream, ")");
    }
}

void fprintf_seg(FILE *stream, char *left, char *right) {
    for (;left <= right; left++) {
        fputc(*left, stream);
    }
}

char *get_end_bracket_ptr(char *start, char *end) {
    assert(start != NULL);
    assert(end != NULL);

    int bracket_balance = 0;

    for (; start < end; start++) {
        if (*start == '(') {
            bracket_balance++;
        } else if (*start == ')') {
            bracket_balance--;
        }
        if (!bracket_balance) {
            return start;
        }
    }
    return NULL;
}

char *get_string_untill_bracket(char *left, char *right, char *bufer) {
    assert(left != NULL);
    assert(right != NULL);
    assert(bufer != NULL);

    while (*left != ')' && *left != '(' && left <= right) {
        *bufer++ = *left++;
    }
    *bufer = '\0';
    return left;
}

bin_tree_elem_t *diff_load_infix_expr(bin_tree_t *tree, bin_tree_elem_t *prev, bool prev_left, char *left, char *right, str_storage_t **storage) {
    assert(tree != NULL);

    char bufer[MINI_BUFER_SZ] = {};
    enum node_types node_type = NODE_VAR;
    long double node_val = 0;
    bin_tree_elem_t *node = NULL;


    if (seg_char_cnt(left, right, '(') == 1 && seg_char_cnt(left, right, ')') == 1) { // leaf
        get_string_untill_bracket(left + 1, right, bufer);

        get_node_type(&node_type, &node_val, bufer);
        node = bin_tree_create_node(NULL, NULL, {node_type});
        node->data.value.fval = node_val;

        printf("leaf : '%s'\n", bufer);
        return node;
    }

    char *left_son_start = left + 1;
    // if (*left_son_start == )
    printf("left_son_start : '%c'", *left_son_start);

    char *left_son_end = get_end_bracket_ptr(left_son_start, right);
    char *node_end_ptr = get_string_untill_bracket(left_son_end + 1, right, bufer);

    char *right_son_start = node_end_ptr;
    char *right_son_end = get_end_bracket_ptr(right_son_start, right);



    get_node_type(&node_type, &node_val, bufer);
    node = bin_tree_create_node(NULL, NULL, {node_type});
    node->data.value.fval = node_val;

    printf("node_operation: '%s'\n", bufer);


    // printf("left_son : '");
    // fprintf_seg(stdout, left_son_start, left_son_end);
    // printf("'\n");
    node->left = diff_load_infix_expr(tree, node, true, left_son_start, left_son_end, storage);


    // printf("right_son : '");
    // fprintf_seg(stdout, right_son_start, right_son_end);
    // printf("'\n");
    node->right = diff_load_infix_expr(tree, node, false, right_son_start, right_son_end, storage);

    node->is_node_left_son = prev_left;
    node->prev = prev;

    // printf("node: [%p]: left[%p] right[%p]\n", node, node->left, node->right);

    return node;
}

int convert_tree_to_dot(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage) {
    assert(dot_code != NULL);
    assert(storage != NULL);
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};

    printf("node : {%d %Lf}\n", node->data.type, node->data.value.fval);

    get_node_string(bufer, node);
    printf("bufer : '%s'\n", bufer);

    size_t label_sz = MAX_NODE_WRAP_SZ + strlen(bufer);
    char *label = get_new_str_ptr(storage, label_sz);
    snprintf(label, label_sz, "{'%s' | {<L> (L)| <R> (R)}}", bufer);
    // printf("label : [%s]\n", label);
    int node_idx = (int) dot_new_node(dot_code, DEFAULT_NODE_PARS, label);
    if (node->data.type == NODE_VAR) {
        dot_code->node_list[node_idx].pars.fillcolor = VAR_COLOR;
    } else if (node->data.type == NODE_FUNC) {
        dot_code->node_list[node_idx].pars.fillcolor = FUNC_COLOR;
    } else if (node->data.type == NODE_NUM) {
        dot_code->node_list[node_idx].pars.fillcolor = NUM_COLOR;
    } else if (node->data.type == NODE_OP) {
        dot_code->node_list[node_idx].pars.fillcolor = OP_COLOR;
    }

    int left_son_idx = -1;
    int right_son_idx = -1;

    if (node->left) {
        left_son_idx = convert_tree_to_dot(node->left, dot_code, storage);
    }
    if (node->right) {
        right_son_idx = convert_tree_to_dot(node->right, dot_code, storage);
    }

    if (left_son_idx != -1) {
        size_t left_edge_idx = dot_new_edge(dot_code, (size_t) node_idx, (size_t) left_son_idx, DEFAULT_EDGE_PARS, "");
        dot_code->edge_list[left_edge_idx].pars.start_suf = "L";
    }
    if (right_son_idx != -1) {
        size_t right_edge_idx = dot_new_edge(dot_code, (size_t) node_idx, (size_t) right_son_idx, DEFAULT_EDGE_PARS, "");
        dot_code->edge_list[right_edge_idx].pars.start_suf = "R";
    }


    return node_idx;
}


void node_dump(FILE *log_file, bin_tree_elem_t *node) {
    assert(log_file != NULL);
    assert(node != NULL);

    char bufer[BUFSIZ] = {};
    size_t indent_sz = 4;
    size_t indent_step = 4;

    fprintf(log_file, "node[%p]\n{\n", node);

    size_t dot_code_pars_block_sz = get_max_str_len(5,
        "left_", "right_",
        "prev_", "is_left_son_",
        "data_"
    );

    fprintf_str_block(log_file, indent_sz, dot_code_pars_block_sz, "left_");
    get_node_string(bufer, node->left);
    fprintf(log_file, " = ([%p]; '%s')\n", node->left, bufer);

    fprintf_str_block(log_file, indent_sz, dot_code_pars_block_sz, "right_");
    get_node_string(bufer, node->right);
    fprintf(log_file, " = ([%p]; '%s')\n", node->right, bufer);

    fprintf_str_block(log_file, indent_sz, dot_code_pars_block_sz, "prev_");
    get_node_string(bufer, node->prev);
    fprintf(log_file, " = ([%p]; '%s')\n", node->prev, bufer);

    fprintf_str_block(log_file, indent_sz, dot_code_pars_block_sz, "is_left_son_");
    fprintf(log_file, " = (%d)\n", node->is_node_left_son);

    fprintf_str_block(log_file, indent_sz, dot_code_pars_block_sz, "data_");
    get_node_string(bufer, node);
    fprintf(log_file, " = ('%s')\n", bufer);

    fprintf(log_file, "}\n");
}

bin_tree_elem_t *differentiate(bin_tree_elem_t *node) {
    assert(node != NULL);

    bin_tree_elem_t *prev_node = node->prev;
    bin_tree_elem_t *new_node  = NULL;
    bin_tree_elem_t *ldiff     = NULL;
    bin_tree_elem_t *rdiff     = NULL;
    bin_tree_elem_t *left      = node->left;
    bin_tree_elem_t *right     = node->right;

    bool is_node_left_son = node->is_node_left_son;

    if (node->data.type == NODE_VAR) {
        new_node = _NUM(1);
    } else if (node->data.type == NODE_NUM) {
        new_node = _NUM(0);
    } else if (node->data.type == NODE_OP) {
        ldiff = differentiate(node->left);
        rdiff = differentiate(node->right);

        if (node->data.value.ival == OP_ADD) {
            new_node = _ADD(ldiff, rdiff);
        } else if (node->data.value.ival == OP_SUB) {
            new_node = _SUB(ldiff, rdiff);
        } else if (node->data.value.ival == OP_MUL) {
            new_node = _ADD(_MUL(ldiff, right), _MUL(left, rdiff));
        } else if (node->data.value.ival == OP_DIV) {
            new_node = _DIV(_SUB(_MUL(ldiff, right), _MUL(left, rdiff)), _MUL(right, right));
        }
    }

    if (node->prev) { // not root
        if (is_node_left_son) {
        prev_node->left = new_node;
        } else {
            prev_node->right = new_node;
        }
    }

    FREE(node);

    return new_node;
}
