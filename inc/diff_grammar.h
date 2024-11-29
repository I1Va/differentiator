#ifndef DIFF_GRAMMAR_H
#define DIFF_GRAMMAR_H

#include "diff_tree.h"
#include "graphviz_funcs.h"
#include "string_funcs.h"


struct parsing_block_t {
    int p;
    char *s;

    bin_tree_t *tree;
    dot_code_t *dot_code;
    str_storage_t ** storage;
};

void draw_parsing_text(parsing_block_t *data);

void SyntaxError();

bin_tree_elem_t *get_N(parsing_block_t *data);
bin_tree_elem_t *get_G(parsing_block_t *data);
bin_tree_elem_t *get_E(parsing_block_t *data);
bin_tree_elem_t *get_T(parsing_block_t *data);
bin_tree_elem_t *get_P(parsing_block_t *data);
bin_tree_elem_t *get_V(parsing_block_t *data);

#endif // DIFF_GRAMMAR_H