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
#define ScannerError(p, c) {debug("ScannerError: text[%d] : '%c'", p, c); fprintf(stderr, WHT); abort();}

bool char_in_str_lex(int c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

token_t next_token(parsing_block_t *data) {
    char *s = data->s;
    int *p = &data->sp;

    int c = data->s[(*p)++];
    long long lval = 0;
    char bufer[MEDIUM_BUFER_SZ] = {};
    size_t bufer_idx = 0;
    char *str = NULL;

    token_t token = {};

    if (isdigit(c)) {
        token.token_type = LEX_NUM;

        while (isdigit(c)) {
            lval = 10 * lval + c - '0';
            c = s[(*p)++];
        }
        (*p)--;

        token.token_val.lval = lval;
        return token;
    }

    if (char_in_str_lex(c)) {
        token.token_type = LEX_STR;
        while (char_in_str_lex(c)) {
            bufer[bufer_idx++] = (char) c;
            c = s[(*p)++];
        }
        (*p)--;

        str = get_new_str_ptr(data->storage, bufer_idx);
        strncpy(str, bufer, bufer_idx);

        token.token_val.sval = str;
        return token;
    }

    switch (c) {
        case '+': return {LEX_ADD}; case '-': return {LEX_SUB};
        case '*': return {LEX_MUL};
        case '(': return {LEX_OBRACE}; case ')': return {LEX_CBRACE};
        case '\n': return {LEX_EOL};
        case ' ': return {LEX_SPACE};
        case '/': return {LEX_DIV};
        case '\t': return {LEX_SPACE};
        case EOF: return {LEX_EOF};
        case '\0': return {LEX_EOF};
        default: ScannerError(*p, s[*p])
    }
    return {LEX_EOF};
}

void token_list_dump(FILE *stream, token_t *token_list, const size_t len) {
    #define LEX_DESCR_(stream, lex, fmt, val) case lex: fprintf(stream, #lex"(" fmt ") ", val); break;

    for (size_t i = 0; i < len; i++) {
        switch (token_list[i].token_type) {
            LEX_DESCR_(stream, LEX_ADD, "%c", '+')
            LEX_DESCR_(stream, LEX_EOF, "%s", "")
            LEX_DESCR_(stream, LEX_NUM, "%Ld", token_list[i].token_val.lval)
            LEX_DESCR_(stream, LEX_MUL, "%c", '*')
            LEX_DESCR_(stream, LEX_SUB, "%c", '-')
            LEX_DESCR_(stream, LEX_OBRACE, "%c", '(')
            LEX_DESCR_(stream, LEX_CBRACE, "%c", ')')
            LEX_DESCR_(stream, LEX_EOL, "%s", "\\n")
            LEX_DESCR_(stream, LEX_STR, "%s", token_list[i].token_val.sval)
            LEX_DESCR_(stream, LEX_SPACE, "%c", ' ')
            default: fprintf(stream, "UNKNOWN_LEX(%d) ", token_list[i].token_type); break;
        }
    }
    fprintf(stream, "\n");

    #undef LEX_DESCR_
}

void lex_scanner(parsing_block_t *data) {
    assert(data != NULL);

    size_t token_idx = 0;

    while (1) {
        token_t token = next_token(data);
        if (token.token_type != LEX_SPACE) {
            data->token_list[token_idx++] = token;
        }
        if (token.token_type == LEX_EOF) {
            break;
        }
    }

    token_list_dump(stdout, data->token_list, token_idx);
}


bin_tree_elem_t *get_G(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);
    bin_tree_elem_t *val = get_E(data);

    if (tl[*tp].token_type != LEX_EOF) {
        SyntaxError(*tp);
    }
    (*tp)++;
    return val;
}

bin_tree_elem_t *get_E(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    bin_tree_elem_t *val = get_T(data);
    while (tl[*tp].token_type == LEX_ADD || tl[*tp].token_type == LEX_SUB) {
        lexemtype op = tl[*tp].token_type;
        (*tp)++;
        bin_tree_elem_t * val2 = get_T(data);

        if (op == LEX_ADD) {
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

    token_t *tl = data->token_list;
    int *tp = &(data->tp);
    bin_tree_elem_t *val = get_P(data);

    while (tl[*tp].token_type == LEX_MUL || tl[*tp].token_type == LEX_DIV) {
        lexemtype op = tl[*tp].token_type;
        (*tp)++;
        bin_tree_elem_t *val2 = get_P(data);
        if (op == LEX_MUL) {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = MUL;
        } else {
            val = bin_tree_create_node(data->tree, NULL, false, val, val2, {OP});
            val->data.value.ival = DIV;
        }
        printf("type: (%d)\n", tl[*tp].token_type);
    }

    return val;
}

bin_tree_elem_t *get_P(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    if (tl[*tp].token_type == LEX_OBRACE) {
        (*tp)++;
        bin_tree_elem_t *val = get_E(data);
        if (tl[*tp].token_type != LEX_CBRACE) {
            SyntaxError(*tp);
        }
        (*tp)++;
        return val;
    } else if (tl[*tp].token_type == LEX_STR && strcmp(tl[*tp].token_val.sval, "x") == 0) {
        return get_V(data);
    } else {
        return get_N(data);
    }
}

bin_tree_elem_t *get_N(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    int val = 0;
    if (tl[*tp].token_type != LEX_NUM) {
        SyntaxError(*tp);
    }
    (*tp)++;

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
//         SyntaxError(*tp);
//     }

//     return val;
// }

bin_tree_elem_t *get_V(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    if (tl[*tp].token_type != LEX_STR && strcmp(tl[*tp].token_val.sval, "x") != 0) {
        SyntaxError(*tp);
    }
    (*tp)++;

    bin_tree_elem_t *res = bin_tree_create_node(data->tree, NULL, false, NULL, NULL, {VAR});
    res->data.value.ival = 0;

    return res;
}
