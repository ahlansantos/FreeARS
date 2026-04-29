#include "fs.h"

#define MAX_FILES 32
#define MAX_NAME 32
#define MAX_SIZE 256

typedef struct {
    char name[MAX_NAME];
    char content[MAX_SIZE];
    int used;
} file_t;

static file_t files[MAX_FILES];

void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].name[0] = '\0';
    }
}

int fs_mkfile(const char *name, const char *content) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            int j;
            for (j = 0; name[j] && j < MAX_NAME - 1; j++)
                files[i].name[j] = name[j];
            files[i].name[j] = '\0';
            for (j = 0; content[j] && j < MAX_SIZE - 1; j++)
                files[i].content[j] = content[j];
            files[i].content[j] = '\0';
            files[i].used = 1;
            return 0;
        }
    }
    return -1;
}

int fs_cat(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            int match = 1;
            for (int j = 0; name[j] || files[i].name[j]; j++) {
                if (name[j] != files[i].name[j]) { match = 0; break; }
            }
            if (match) return i;
        }
    }
    return -1;
}

int fs_ls() {
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++)
        if (files[i].used) count++;
    return count;
}

int fs_rm(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            int match = 1;
            for (int j = 0; name[j] || files[i].name[j]; j++) {
                if (name[j] != files[i].name[j]) { match = 0; break; }
            }
            if (match) {
                files[i].used = 0;
                files[i].name[0] = '\0';
                return 0;
            }
        }
    }
    return -1;
}

char *fs_get_name(int index) {
    if (index < 0 || index >= MAX_FILES || !files[index].used) return 0;
    return files[index].name;
}

char *fs_get_content(int index) {
    if (index < 0 || index >= MAX_FILES || !files[index].used) return 0;
    return files[index].content;
}

int fs_get_index(const char *name) {
    return fs_cat(name);
}