#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "diff_grammar.h"
#include "diff_funcs.h"
#include "diff_tree.h"
#include "general.h"
#include "graphviz_funcs.h"
#include "string_funcs.h"

// void SyntaxError() {
//     abort();
// }

#define SyntaxError(p) {debug("SyntaxError: p=%d", p); fprintf(stderr, WHT); abort();}

enum lexemtype{
    LEX_ID = 0,
    LEX_NUM = 1,
    LEX_ADD = 2,
    LEX_MUL = 3,
    LEX_SUB = 4,
};

union token_value_t {
    int ival;
    long double dval;
    char *sval;
};

struct token_t {
    enum lexemtype token_type;
    union token_value_t token_val;
};

// void lex_scanner(char *text, const size_t len) {
//     const size_t TOKEN_LIST_MX_SZ = 256;
//     token_t token_list[TOKEN_LIST_MX_SZ];
//     size_t token_idx = 0;

//     size_t p = 0;
//     while(1) {
//         int c = text[p++];
//         if(isdigit(c)) {
//             int val = c - '0';
//             while(isdigit(c = text[p++]))
//                 val = (10 * val) + c - '0';
//             token_list[token_idx].token_type = LEX_NUM;
//             token_list[token_idx].token_val.ival = val;
//         }
//     }


//     // if(c == EOF) seeneof = 1;
//     // else ungetc(c, yyin);
//     // return NUMBER;
//     // }
//     // switch(c) {
//     // case '+': return ADD; case '-': return SUB;
//     // case '*': return MUL; case '|': return ABS;
//     // case '(': return OP; case ')': return CP;
//     // case '\n': return EOL;
//     // case ' ': case '\t': break; /* ignore these */
//     // case EOF: return 0; /* standard end-of-file token */
//     // case '/': c = getc(yyin);
//     // if(c == '/') { /* it's a comment */
//     // while((c = getc(yyin)) != '\n')
//     // if(c == EOF) return 0; /* EOF in comment line */
//     // break;
//     // }
//     // if(c == EOF) seeneof = 1; /* it's division */
//     // else ungetc(c, yyin);
//     // return DIV;
//     // default: yyerror("Mystery character %c\n", c); break

//     // }

// }






bin_tree_elem_t *get_G(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    bin_tree_elem_t *val = get_E(data);

    if (s[*p] != '\0') {
        SyntaxError(*p);
    }
    (*p)++;
    return val;
}

bin_tree_elem_t *get_E(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    bin_tree_elem_t *val = get_T(data);
    while (s[*p] == '+' || s[*p] == '-') {
        int op = s[*p];
        (*p)++;
        bin_tree_elem_t * val2 = get_T(data);

        if (op == '+') {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = ADD;
        } else {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = SUB;

        }
    }

    return val;
}

bin_tree_elem_t *get_T(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    bin_tree_elem_t *val = get_P(data);

    while (s[*p] == '*' || s[*p] == '/') {
        int op = s[*p];
        (*p)++;
        bin_tree_elem_t *val2 = get_P(data);
        if (op == '*') {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = MUL;
        } else {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = DIV;
        }
    }

    return val;
}

bin_tree_elem_t *get_P(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);

    if (s[*p] == '(') {
        (*p)++;
        bin_tree_elem_t *val = get_E(data);
        if (s[*p] != ')') {
            SyntaxError(*p);
        }
        (*p)++;
        return val;
    } else if (s[*p] == 'x') {
        return get_V(data);
    } else {
        return get_N(data);
    }
}

bin_tree_elem_t *get_N(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    int old_p = *p;

    int val = 0;
    while ('0' <= s[*p] && s[*p] <= '9') {
        val = val * 10 + s[*p] - '0';
        (*p)++;
    }
    if (old_p == *p) {
        SyntaxError(*p);
    }
    bin_tree_elem_t *node = bin_tree_create_node(data->tree, NULL, false, NULL, NULL, {NUM});
    node->data.value.ival = val;
    return node;
}

// int get_S(parsing_block_t *data) {
//     assert(data != NULL);

//     char *s = data->s;
//     int *p = &(data->p);
//     int old_p = *p;

//     int val = 0;
//     while (('0' <= s[*p] && s[*p] <= '9') || ('a' <= s[*p] && s[*p] <= 'z') || ('A' <= s[*p] && s[*p] <= 'z')) {
//         val = val * 10 + s[*p] - '0';
//         (*p)++;
//     }
//     if (old_p == *p) {
//         SyntaxError(*p);
//     }

//     return val;
// }

bin_tree_elem_t *get_V(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);

    if (s[*p] != 'x') {
        SyntaxError(*p);
    }
    (*p)++;

    bin_tree_elem_t *res = bin_tree_create_node(data->tree, NULL, false, NULL, NULL, {VAR});
    res->data.value.ival = 0;

    return res;
}
