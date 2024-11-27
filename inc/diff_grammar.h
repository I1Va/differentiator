#ifndef DIFF_GRAMMAR_H
#define DIFF_GRAMMAR_H

struct parsing_block_t {
    int p;
    char *s;
};

void SyntaxError();
int GetN(parsing_block_t *data);
int GetG(parsing_block_t *data);
int GetE(parsing_block_t *data);
int GetT(parsing_block_t *data);
int GetP(parsing_block_t *data);

#endif // DIFF_GRAMMAR_H