#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "diff_grammar.h"

#include "general.h"

// void SyntaxError() {
//     abort();
// }

#define SyntaxError() {debug("SyntaxError"); fprintf(stderr, WHT); abort();}


int GetG(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    int val = GetE(data);

    if (s[*p] != '\0') {
        SyntaxError();
    }
    (*p)++;
    return val;
}

int GetE(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    int val = GetT(data);

    while (s[*p] == '+' || s[*p] == '-') {
        int op = s[*p];
        (*p)++;
        int val2 = GetT(data);
        if (op == '+') {
            val += val2;
        } else {
            val -= val2;
        }
    }

    return val;
}

int GetT(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);
    int val = GetP(data);

    while (s[*p] == '*' || s[*p] == '/') {
        int op = s[*p];
        (*p)++;
        int val2 = GetP(data);
        if (op == '*') {
            val *= val2;
        } else {
            val /= val2;
        }
    }

    return val;
}

int GetP(parsing_block_t *data) {
    assert(data != NULL);

    char *s = data->s;
    int *p = &(data->p);

    if (s[*p] == '(') {
        (*p)++;
        int val = GetE(data);
        if (s[*p] != ')') {
            SyntaxError();
        }
        (*p)++;
        return val;
    } else {
        return GetN(data);
    }
}

int GetN(parsing_block_t *data) {
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
        SyntaxError();
    }

    return val;
}



