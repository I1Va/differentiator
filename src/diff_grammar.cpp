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
#include "diff_DSL.h"
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
        case '^': return {LEX_POW};
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
            LEX_DESCR_(stream, LEX_DIV, "%c", '/')
            LEX_DESCR_(stream, LEX_OBRACE, "%c", '(')
            LEX_DESCR_(stream, LEX_CBRACE, "%c", ')')
            LEX_DESCR_(stream, LEX_EOL, "%s", "\\n")
            LEX_DESCR_(stream, LEX_POW, "%s", "^")
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
        if (token.token_type != LEX_SPACE && token.token_type != LEX_EOL) {
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
            val = _ADD(val, val2);
        } else {
            val = _SUB(val, val2);
        }
    }

    return val;
}

bin_tree_elem_t *get_T(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);
    bin_tree_elem_t *val = get_pow(data);

    while (tl[*tp].token_type == LEX_MUL || tl[*tp].token_type == LEX_DIV) {
        lexemtype op = tl[*tp].token_type;
        (*tp)++;
        bin_tree_elem_t *val2 = get_pow(data);
        if (op == LEX_MUL) {
            val = _MUL(val, val2);
        } else {
            val = _DIV(val, val2);
        }
        // printf("type: (%d)\n", tl[*tp].token_type);
    }

    return val;
}

bin_tree_elem_t *get_pow(parsing_block_t *data) {
    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    bin_tree_elem_t *left = get_P(data);

    if (tl[*tp].token_type == LEX_POW) {
        (*tp)++;

        bin_tree_elem_t *right = get_pow(data);

        left = _POW(left, right);
    }

    return left;
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
    } else if (tl[*tp].token_type == LEX_STR && tl[(*tp) + 1].token_type == LEX_OBRACE) {
        return get_F(data);
    } else if (tl[*tp].token_type == LEX_STR && strcmp(tl[*tp].token_val.sval, "x") == 0) {
        return get_V(data);
    } else if (tl[*tp].token_type == LEX_NUM) {
        return get_N(data);
    } else {
        SyntaxError(*tp)
        return NULL;
    }
}

bin_tree_elem_t *get_N(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    long long val = 0;
    if (tl[*tp].token_type != LEX_NUM) {
        SyntaxError(*tp);
    }

    val = tl[*tp].token_val.lval;
    (*tp)++;

    return _NUM(val);
}

bin_tree_elem_t *get_F(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    if (tl[*tp].token_type == LEX_STR && tl[(*tp) + 1].token_type == LEX_OBRACE) {
        char *func_name = tl[*tp].token_val.sval;

        (*tp) += 2;
        bin_tree_elem_t *val = get_E(data);
        printf("E : '%d'\n", val->data.type);
        if (tl[*tp].token_type != LEX_CBRACE) {
            SyntaxError(*tp);
        }
        (*tp)++;

        return _FUNC(val, func_name);
    } else {
        SyntaxError(*tp);
        return NULL;
    }
}

bin_tree_elem_t *get_V(parsing_block_t *data) {
    assert(data != NULL);

    token_t *tl = data->token_list;
    int *tp = &(data->tp);

    if (tl[*tp].token_type != LEX_STR && strcmp(tl[*tp].token_val.sval, "x") != 0) {
        SyntaxError(*tp);
    }
    (*tp)++;

    return _VAR();
}
