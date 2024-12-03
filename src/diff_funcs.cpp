#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "diff_tree.h"
#include "diff_funcs.h"
#include "error_processing.h"
#include "stack_funcs.h"
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

size_t put_node_in_dotcode(bin_tree_elem_t *node, dot_code_t *dot_code, str_storage_t **storage) {
    assert(dot_code != NULL);
    assert(storage != NULL);
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);
    // printf("bufer : '%s'\n", bufer);

    size_t label_sz = MAX_NODE_WRAP_SZ + strlen(bufer);
    char *label = get_new_str_ptr(storage, label_sz);
    snprintf(label, label_sz, "{'%s' | {<L> (L)| <R> (R)}}", bufer);
    // printf("label : [%s]\n", label);

    size_t node_idx = dot_new_node(dot_code, DEFAULT_NODE_PARS, label);

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

    size_t node_idx = put_node_in_dotcode(node, dot_code, storage);

    int left_son_idx = -1;
    int right_son_idx = -1;

    if (node->left) {
        left_son_idx = convert_subtree_to_dot(node->left, dot_code, storage);
    }
    if (node->right) {
        right_son_idx = convert_subtree_to_dot(node->right, dot_code, storage);
    }

    if (left_son_idx != -1) {
        size_t left_edge_idx = dot_new_edge(dot_code, (size_t) node_idx, (size_t) left_son_idx, DEFAULT_EDGE_PARS, "");
        dot_code->edge_list[left_edge_idx].pars.start_suf = "L";
    }
    if (right_son_idx != -1) {
        size_t right_edge_idx = dot_new_edge(dot_code, (size_t) node_idx, (size_t) right_son_idx, DEFAULT_EDGE_PARS, "");
        dot_code->edge_list[right_edge_idx].pars.start_suf = "R";
    }

    return (int) node_idx;
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
    // fprintf(log_file, " = ([%p]; '%s')\n", node->prev, bufer);

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

bin_tree_elem_t *differentiate(bin_tree_elem_t *node) {
    assert(node != NULL);

    bin_tree_elem_t *new_node  = NULL;

    if (node->data.type == NODE_VAR) {
        FREE(node);
        return _NUM(1);
    } else if (node->data.type == NODE_NUM) {
        FREE(node);
        return _NUM(0);
    } else if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_ADD) {
            new_node = _ADD(differentiate(node->left), differentiate(node->right));
        } else if (node->data.value.ival == OP_SUB) {
            new_node = _SUB(differentiate(node->left), differentiate(node->right));
        } else if (node->data.value.ival == OP_MUL) {

            bin_tree_elem_t *left_copy = get_tree_copy(node->left);
            bin_tree_elem_t *right_copy = get_tree_copy(node->right);
            bin_tree_elem_t *left_diff = differentiate(left_copy);
            bin_tree_elem_t *right_diff = differentiate(right_copy);

            new_node =
            _ADD(
                _MUL(left_diff, node->right),
                _MUL(right_diff, node->left)
            );
        } else if (node->data.value.ival == OP_DIV) {
            bin_tree_elem_t *left_copy = get_tree_copy(node->left);
            bin_tree_elem_t *right_copy = get_tree_copy(node->right);
            bin_tree_elem_t *left_diff = differentiate(left_copy);
            bin_tree_elem_t *right_diff = differentiate(right_copy);

            new_node = _DIV
            (
                _SUB(_MUL(left_diff, node->right), _MUL(node->left, right_diff)),
                _MUL(get_tree_copy(node->right), get_tree_copy(node->right))
            );
        }
    } else if (node->data.type == NODE_FUNC) {
        if (strcmp(node->data.value.sval, "sin") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _FUNC(node->right, "cos"));
        } else if (strcmp(node->data.value.sval, "cos") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _MUL(_NUM(-1), _FUNC(node->right, "sin")));
        } else if (strcmp(node->data.value.sval, "ln") == 0) {
            new_node = _MUL(differentiate(get_tree_copy(node->right)), _DIV(_NUM(1), node->right));
        } else if (strcmp(node->data.value.sval, "sqrt") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1), _MUL(_NUM(2), get_tree_copy(node))));
        } else if (strcmp(node->data.value.sval, "tg") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1),
                _MUL(_FUNC(get_tree_copy(node->right), "cos"), _FUNC(get_tree_copy(node->right), "cos"))));
        } else if (strcmp(node->data.value.sval, "ctg") == 0) {
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(-1),
                _MUL(_FUNC(get_tree_copy(node->right), "sin"), _FUNC(get_tree_copy(node->right), "sin"))));
        } else if (strcmp(node->data.value.sval, "arcsin") == 0) {
            bin_tree_elem_t *radical_node = _SUB(_NUM(1), _MUL(get_tree_copy(node->right), get_tree_copy(node->right)));
            new_node = _MUL(differentiate(node->right), _DIV(_NUM(1), _FUNC(radical_node, "sqrt")));
        }
    }

    FREE(node);
    return new_node;
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

void write_subtree_to_latex_code(FILE *stream, bin_tree_elem_t *node) {
    assert(node != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    get_node_string(bufer, node);

    if (node->data.type == NODE_VAR) {
        fprintf(stream, "%s", bufer);
        return;
    }

    if (node->data.type == NODE_NUM) {

        if (node->data.value.lval < 0) {
            fprintf(stream, "(%s)", bufer);
        } else {
            fprintf(stream, "%s", bufer);
        }
        return;
    }

    if (node->data.type == NODE_OP) {
        if (node->data.value.ival == OP_DIV) {
            fprintf(stream, "\\frac{");
            if (node->left) {
                write_subtree_to_latex_code(stream, node->left);
            }
            fprintf(stream, "}{");
            if (node->right) {
                write_subtree_to_latex_code(stream, node->right);
            }
            fprintf(stream, "}");
        } else {
            bool left_const_state = (node->left->data.type == NODE_NUM);
            bool right_const_state = (node->right->data.type == NODE_NUM);

            bool left_var_state = (node->left->data.type == NODE_VAR);
            bool right_var_state = (node->right->data.type == NODE_VAR);

            if (left_const_state && right_var_state) {
                write_subtree_to_latex_code(stream, node->left);
                write_subtree_to_latex_code(stream, node->right);
            } else if (right_const_state && left_var_state) {
                write_subtree_to_latex_code(stream, node->right);
                write_subtree_to_latex_code(stream, node->left);
            } else {
                if (node->left) {
                    write_subtree_to_latex_code(stream, node->left);
                }

                fprintf(stream, "%s", bufer);

                if (node->right) {
                    write_subtree_to_latex_code(stream, node->right);
                }
            }
        }

        return;
    }

    if (node->data.type == NODE_FUNC) {
        fprintf(stream, "%s(", bufer);
        write_subtree_to_latex_code(stream, node->right);
        fprintf(stream, ")");
    }
}

bool make_tex_of_subtree(const char dir[], const char name[], bin_tree_elem_t *root) {
    assert(dir != NULL);
    assert(root != NULL);

    char bufer[MEDIUM_BUFER_SZ] = {};
    snprintf(bufer, MEDIUM_BUFER_SZ, "%s/%s", dir, name);

    FILE *latex_code_file = fopen(bufer, "w");
    if (!latex_code_file) {
        debug("can't open : '%s'", bufer);
        return false;
    }

    fprintf(latex_code_file, "\\documentclass[12pt]{article}\n\\begin{document}\n");
    fprintf(latex_code_file, "$");
    write_subtree_to_latex_code(latex_code_file, root);
    fprintf(latex_code_file, "$");
    fprintf(latex_code_file, "\n\\end{document}\n");

    fclose(latex_code_file);


    snprintf(bufer, MEDIUM_BUFER_SZ, "cd %s && pdflatex %s", dir, name);
    system(bufer);

    return true;
}