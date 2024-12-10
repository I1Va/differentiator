#include <assert.h>

#include "diff_funcs.h"
#include "diff_tree.h"
#include "tex_funcs.h"

const char *MATH_PHRASES[] =
{
    "Every Soviet schoolchild understands",
    "Obviously",
    "It's not hard to notice",
};

const size_t PHRASES_CNT = sizeof(MATH_PHRASES) / sizeof(const char *);

bool tex_dir_ctor(tex_dir_t *tex_dir, const char dir[], const char name[]) {
    assert(dir != NULL);
    assert(name != NULL);
    assert(tex_dir != NULL);

    strncpy(tex_dir->dir, dir, MEDIUM_BUFER_SZ);
    strncpy(tex_dir->name, name, MEDIUM_BUFER_SZ);
    snprintf(tex_dir->code_path, MEDIUM_BUFER_SZ, "%s/%s", dir, name);

    tex_dir->code_file_ptr = fopen(tex_dir->code_path, "w");
    if (!tex_dir->code_file_ptr) {
        debug("can't open : '%s'", tex_dir->code_path);
        return false;
    }

    return true;
}

void tex_dir_dtor(tex_dir_t *tex_dir) {
    if (tex_dir && tex_dir->code_file_ptr) {
        fclose(tex_dir->code_file_ptr);
    }
}

void tex_start_code(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "\\documentclass[12pt]{article}\n\\begin{document}\n");
}

void tex_close_code(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "\\end{document}\n");
    fclose(tex_dir->code_file_ptr);
}

void tex_generate_pdf(tex_dir_t *tex_dir) {
    tex_close_code(tex_dir);

    char bufer[MEDIUM_BUFER_SZ] = {};
    snprintf(bufer, MEDIUM_BUFER_SZ, "cd %s && pdflatex %s", tex_dir->dir, tex_dir->name);
    printf("bufer : '%s'\n", bufer);
    system(bufer);
}

void latex_insert_phrase(tex_dir_t *tex_dir) {
    fprintf(tex_dir->code_file_ptr, "%s \\\\ \n", MATH_PHRASES[((size_t) rand()) % PHRASES_CNT]);
}