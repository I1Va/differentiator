#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "general.h"
#include "diff_tree.h"
#include "diff_funcs.h"
#include "error_processing.h"
#include "stack_funcs.h"
#include "string_funcs.h"
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

int put_node_in_dotcode(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage) {
    assert(dot_code != NULL);
    assert(storage != NULL);
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);

    size_t label_sz = MAX_NODE_WRAP_SZ + strlen(bufer);
    char *label = get_new_str_ptr(storage, label_sz);
    snprintf(label, label_sz, "{'%s' | {<L> (L)| <R> (R)}}", bufer);
    // printf("label : [%s]\n", label);

    int node_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, label);

    if (node->data.type == NODE_VAR) {
        dot_code->node_list[node_idx].pars.fillcolor = VAR_COLOR;
    } else if (node->data.type == NODE_FUNC) {
        dot_code->node_list[node_idx].pars.fillcolor = FUNC_COLOR;
    } else if (node->data.type == NODE_NUM) {
        dot_code->node_list[node_idx].pars.fillcolor = NUM_COLOR;
    } else if (node->data.type == NODE_OP) {
        dot_code->node_list[node_idx].pars.fillcolor = OP_COLOR;
    }

    return node_idx;
}

int convert_subtree_to_dot(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage) {
    assert(dot_code != NULL);
    assert(storage != NULL);
    assert(node != NULL);

    node->graphviz_idx = put_node_in_dotcode(node, dot_code, storage);

    int left_son_idx = -1;
    int right_son_idx = -1;

    if (node->left) {
        left_son_idx = convert_subtree_to_dot(node->left, dot_code, storage);
        if (left_son_idx == -1) {
            debug("dot_code overflow");
            return -1;
        }
    }
    if (node->right) {
        right_son_idx = convert_subtree_to_dot(node->right, dot_code, storage);
        if (right_son_idx == -1) {
            debug("dot_code overflow");
            return -1;
        }
    }

    if (left_son_idx != -1) {
        int left_edge_idx = dot_new_edge(dot_code, (size_t) node->graphviz_idx, (size_t) left_son_idx, DEFAULT_EDGE_PARS, "");
        if (left_edge_idx == -1) {
            debug("dot_code overflow");
            return -1;
        }
        dot_code->edge_list[left_edge_idx].pars.start_suf = "L";
    }
    if (right_son_idx != -1) {
        int right_edge_idx = dot_new_edge(dot_code, (size_t) node->graphviz_idx, (size_t) right_son_idx, DEFAULT_EDGE_PARS, "");
        if (right_edge_idx == -1) {
            debug("dot_code overflow");
            return -1;
        }
        dot_code->edge_list[right_edge_idx].pars.start_suf = "R";
    }

    return (int) node->graphviz_idx;
}

bool convert_tree_to_dot(bin_tree_t *tree, dot_code_t *dot_code, str_storage_t **storage) {
    assert(dot_code != NULL);
    assert(storage != NULL);
    assert(tree != NULL);

    stk_err return_err = STK_ERR_OK;

    for (size_t i = 0; i < tree->node_stack.size; i++) {
        bin_tree_elem_t *node = *((bin_tree_elem_t **) stack_get_elem(&tree->node_stack, i, &return_err));

        if (return_err != STK_ERR_OK) {
            debug("stack_get_elem[%lu] failed", i);
            return false;
        }

        node->graphviz_idx = (int) put_node_in_dotcode(node, dot_code, storage);
    }

    for (size_t i = 0; i < tree->node_stack.size; i++) {
        bin_tree_elem_t *node = *((bin_tree_elem_t **) stack_get_elem(&tree->node_stack, i, &return_err));

        // printf("node[%p] = {%d}\n", node, node->graphviz_idx);
        // node_dump(stdout, node);

        if (return_err != STK_ERR_OK) {
            debug("stack_get_elem[%lu] failed", i);
            return false;
        }


        if (node->left) {
            size_t left_edge_idx = dot_new_edge(dot_code, (size_t) node->graphviz_idx, (size_t) node->left->graphviz_idx, DEFAULT_EDGE_PARS, "");
            // printf("%d -> %d\n", node->graphviz_idx, node->left->graphviz_idx);
            dot_code->edge_list[left_edge_idx].pars.start_suf = "L";
        }
        if (node->right) {
            size_t right_edge_idx = dot_new_edge(dot_code, (size_t) node->graphviz_idx, (size_t) node->right->graphviz_idx, DEFAULT_EDGE_PARS, "");
            // printf("%d -> %d\n", node->graphviz_idx, node->right->graphviz_idx);
            dot_code->edge_list[right_edge_idx].pars.start_suf = "R";
        };
    }

    return true;
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

void write_infix(bin_tree_elem_t *node) {
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);

    if (node->data.type == NODE_VAR) {
        printf("%s", bufer);
        return;
    }

    if (node->data.type == NODE_NUM) {

        if (node->data.value.lval < 0) {
            printf("(%s)", bufer);
        } else {
            printf("%s", bufer);
        }
        return;
    }

    if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_DIV) {
            printf("(");
            if (node->left) {
                write_infix(node->left);
            }
            printf(")");

            printf("%s", bufer);

            printf("(");
            if (node->right) {
                write_infix(node->right);
            }
            printf(")");
        } else {
            if (node->left) {
                write_infix(node->left);
            }

            printf("%s", bufer);

            if (node->right) {
                write_infix(node->right);
            }
        }

        return;
    }

    if (node->data.type == NODE_FUNC) {
        printf("%s(", bufer);
        write_infix(node->right);
        printf(")");
    }
}

bin_tree_elem_t *neutrals_remove_diff_tree(bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->data.type == NODE_NUM) {
        return node;
    }
    if (node->data.type == NODE_VAR) {
        return node;
    }

    if (node->data.type == NODE_FUNC) {
        node->right = neutrals_remove_diff_tree(node->right);
        return node;
    }

    if (node->data.type == NODE_OP) {
        assert(node->left != NULL);
        assert(node->right != NULL);

        bin_tree_elem_t *left = neutrals_remove_diff_tree(node->left);
        bin_tree_elem_t *right = neutrals_remove_diff_tree(node->right);

        node->left = left;
        node->right = right;

        assert(left != NULL);
        assert(right != NULL);

        bin_tree_elem_t *new_node = NULL;

        if (node->data.value.ival == OP_MUL) {
            if (left->data.type == NODE_NUM && left->data.value.lval == 1) {
                new_node = right;
                FREE(left);
                FREE(node)
            } else if (right->data.type == NODE_NUM && right->data.value.lval == 1) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_ADD) {
            if (left->data.type == NODE_NUM && left->data.value.lval == 0) {
                new_node = right;
                FREE(left);
                FREE(node)
            } else if (right->data.type == NODE_NUM && right->data.value.lval == 0) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_DIV) {
            if (right->data.type == NODE_NUM && right->data.value.lval == 1) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

        if (node->data.value.ival == OP_SUB) {
            if (right->data.type == NODE_NUM && right->data.value.lval == 0) {
                new_node = left;
                FREE(right);
                FREE(node);
            } else {
                new_node = node;
            }
            return new_node;
        }

    }

    debug("unknown node_type : {%d}", node->data.type);
    return NULL;
}

bin_tree_elem_t *roll_up_null_mult(bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->data.type == NODE_OP && node->data.value.ival == OP_MUL) {
        bool left_null_state = (node->left->data.type == NODE_NUM) && (node->left->data.value.lval == 0);
        bool right_null_state = (node->right->data.type == NODE_NUM) && (node->right->data.value.lval == 0);

        if (left_null_state || right_null_state) {
            bin_tree_elem_t *new_node = _NUM(0);
            new_node->constant_state = true;

            sub_tree_dtor(node);
            return new_node;
        }
    }

    if (node->data.type == NODE_OP && node->data.value.ival == OP_DIV) {
        bool left_null_state = node->left->data.type == NODE_NUM && node->left->data.value.lval == 0;
        if (left_null_state) {
            bin_tree_elem_t *new_node = _NUM(0);
            new_node->constant_state = true;

            sub_tree_dtor(node);
            return new_node;
        }
    }

    return node;
}

bin_tree_elem_t *constant_convolution_diff_tree(bin_tree_elem_t *node) {
    assert(node != NULL);

    if (node->data.type == NODE_NUM) {
        node->constant_state = true;
        return node;
    }
    if (node->data.type == NODE_VAR) {
        return node;
    }

    if (node->data.type == NODE_FUNC) {
        node->right = constant_convolution_diff_tree(node->right);
        return node;
    }

    if (node->data.type == NODE_OP) {
        assert(node->left != NULL);
        assert(node->right != NULL);

        bin_tree_elem_t *left = constant_convolution_diff_tree(node->left);
        bin_tree_elem_t *right = constant_convolution_diff_tree(node->right);

        node->left = left;
        node->right = right;

        bin_tree_elem_t *new_node = NULL;



        if (left->constant_state && right->constant_state) {
            assert(left->data.type == NODE_NUM);
            assert(right->data.type == NODE_NUM);

            if (node->data.value.ival == OP_MUL) {
                new_node = _NUM(left->data.value.lval * right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_ADD) {
                new_node = _NUM(left->data.value.lval + right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_DIV) {
                assert(right->data.value.lval != 0);

                new_node = _NUM(left->data.value.lval / right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else if (node->data.value.ival == OP_SUB) {
                new_node = _NUM(left->data.value.lval - right->data.value.lval);
                FREE(left);
                FREE(right);
                FREE(node)
            } else {
                debug("unknown op {%d}", node->data.value.ival);
            }
            new_node->constant_state = true;
            return new_node;
        }

        return roll_up_null_mult(node);
    }

    debug("unknown node_type : {%d}", node->data.type);
    return NULL;
}

defer_info_t defer_info_t_ctor(dot_code_t *dot_code) {
    defer_info_t defer_info = {};
    defer_info.tree_scale_val = 0.0;
    defer_info.dot_code = dot_code;
    defer_info.def_list_idx = 0;

    defer_info.letter = 'A';
    defer_info.letter_idx = 0;

    defer_info.defer_state = true;
    defer_info.defer_lowerbound = DEFER_LOWERBOUND;
    defer_info.low_delta = DEFER_LOW_DELTA;
    defer_info.high_delta = DEFER_HIGH_DELTA;

    return defer_info;
}

double def_coef_get(double scale_val) {
    return (int) (scale_val * 0.7);
}

double calc_subtree_scale_val(subtree_info_t info) {
    return (int) info.height;
}

bool defer_check(bin_tree_elem_t *node, defer_info_t *defer_info) {
    if (!defer_info->defer_state) {
        return false;
    }

    double node_scale_val = calc_subtree_scale_val(node->subtree_info);
    double tree_scale_val = defer_info->tree_scale_val;

    double proportion = node_scale_val / tree_scale_val;

    if (node_scale_val < DEFER_LOWERBOUND) {
        return false;
    }

    return (proportion > DEFER_LOW_DELTA && proportion < DEFER_HIGH_DELTA);
}

subtree_info_t get_node_info(bin_tree_elem_t *root) {
    subtree_info_t info = {};

    info.sz = 1;
    info.height = 1;
    if (root->data.type == NODE_OP && root->data.value.ival == OP_DIV) {
        info.divop_cnt = 1;
    }
    if (root->data.type == NODE_OP && root->data.value.ival == OP_ADD) {
        info.addop_cnt = 1;
    }
    if (root->data.type == NODE_OP && root->data.value.ival == OP_SUB) {
        info.subop_cnt = 1;
    }
    if (root->data.type == NODE_OP && root->data.value.ival == OP_MUL) {
        info.mulop_cnt = 1;
    }

    return info;
}

void merge_subtrees_info(subtree_info_t *dest, subtree_info_t src) {
    dest->addop_cnt += src.addop_cnt;
    dest->divop_cnt += src.divop_cnt;
    dest->mulop_cnt += src.mulop_cnt;
    dest->subop_cnt += src.subop_cnt;

    dest->sz += src.sz;

    dest->height = MAX(dest->height, src.height + 1);
}

void collect_tree_info(bin_tree_elem_t *root) {
    assert(root);

    root->subtree_info = get_node_info(root);

    if (root->left) {
        collect_tree_info(root->left);
        merge_subtrees_info(&root->subtree_info, root->left->subtree_info);
    }
    if (root->right) {
        collect_tree_info(root->right);
        merge_subtrees_info(&root->subtree_info, root->right->subtree_info);
    }
}