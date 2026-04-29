#ifndef FS_H
#define FS_H
#include <stdint.h>

void fs_init();
int fs_mkfile(const char *name, const char *content);
int fs_cat(const char *name);
int fs_ls();
int fs_rm(const char *name);
char *fs_get_name(int index);
char *fs_get_content(int index);
int fs_get_index(const char *name);

#endif