#ifndef DIFF_GRAMMAR_H
#define DIFF_GRAMMAR_H

#include "diff_tree.h"
#include "graphviz_funcs.h"
#include "string_funcs.h"

const size_t TOKEN_LIST_MAX_SZ = 1028;

union token_value_t {
    int ival;
    long long lval;
    long double dval;
    char *sval;
};

enum lexemtype {
    LEX_EOF = 0,
    LEX_NUM = 1,
    LEX_ADD = 2,
    LEX_MUL = 3,
    LEX_SUB = 4,
    LEX_OBRACE = 5,
    LEX_CBRACE = 6,
    LEX_EOL = 7,
    LEX_SPACE = 9,
    LEX_STR = 10,
    LEX_DIV = 11,
};

struct token_t {
    enum lexemtype token_type;
    union token_value_t token_val;
};

struct parsing_block_t {
    int p;

    char *s;
    token_t *token_list;
    bin_tree_t *tree;
    dot_code_t *dot_code;
    str_storage_t ** storage;
};

void draw_parsing_text(parsing_block_t *data);

void SyntaxError();



token_t next_token(parsing_block_t *data);

void token_list_dump(FILE *stream, token_t *token_list, const size_t len);

void lex_scanner(parsing_block_t *data);

bin_tree_elem_t *get_N(parsing_block_t *data);
bin_tree_elem_t *get_G(parsing_block_t *data);
bin_tree_elem_t *get_E(parsing_block_t *data);
bin_tree_elem_t *get_T(parsing_block_t *data);
bin_tree_elem_t *get_P(parsing_block_t *data);
bin_tree_elem_t *get_V(parsing_block_t *data);

#endif // DIFF_GRAMMAR_H