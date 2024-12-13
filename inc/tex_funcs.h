#ifndef TEX_FUNCS_H
#define TEX_FUNCS_H

#include "diff_funcs.h"

struct tex_dir_t {
    char dir[MEDIUM_BUFER_SZ];
    char name[MEDIUM_BUFER_SZ];
    char code_path[MEDIUM_BUFER_SZ];
    FILE *code_file_ptr;
};

struct tex_info_t {
    bool frac_state;
    bool mul_cdot_state;
};

bool tex_dir_ctor(tex_dir_t *tex_dir, const char dir[], const char name[]);
void tex_dir_dtor(tex_dir_t *tex_dir);
void tex_start_code(tex_dir_t *tex_dir);
void tex_close_code(tex_dir_t *tex_dir);
void tex_generate_pdf(tex_dir_t *tex_dir);
void latex_insert_phrase(tex_dir_t *tex_dir);
void write_subtree(FILE *stream, bin_tree_elem_t *node, defer_info_t *defer_info, tex_info_t tex_info);
bool make_tex_of_subtree(tex_dir_t *tex_dir, bin_tree_elem_t *root, defer_info_t defer_info, tex_info_t tex_info);

#endif // TEX_FUNCS_H