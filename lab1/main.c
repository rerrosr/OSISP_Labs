#define _GNU_SOURCE
#define MAX_PATH_LENGTH 255

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <locale.h>

void traverseDirectory(char *path, char *flags);

int compareEntries(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

int main(int argc, char *argv[]) {
    char *flags = (char *)calloc(6, sizeof(char)); 
    char *path = ".";

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strstr(argv[i], "-"))
                strcat(flags, argv[i]);
            else if (argv[i][0] == '/')
                path = argv[i];
        }
    } else
        strcat(flags, "-ldf");

    if (!strstr(flags, "l") && !strstr(flags, "d") && !strstr(flags, "f"))
        strcat(flags, "-ldf");

    if (strstr(flags, "d"))
        printf("d - %s/\n", path);

    traverseDirectory(path, flags);
    free(flags);
    return 0;
}

void traverseDirectory(char *path, char *flags) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat st;
    char **entries = NULL;
    size_t entryCount = 0;

    if (!dir) {
        if (errno) {
            fprintf(stderr, "Error: Permission denied for directory '%s'\n", path);
            errno = 0;
            return;
        }
    }

    
    while ((entry = readdir(dir))) {
        if (!(strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))) {
            continue;
        }
        char tmp[MAX_PATH_LENGTH];
        strcpy(tmp, path);
        strcat(tmp, "/");
        strcat(tmp, entry->d_name);
        entries = realloc(entries, (entryCount + 1) * sizeof(char *));
        entries[entryCount] = strdup(tmp);
        entryCount++;
    }

    if (strstr(flags, "s")) {
        setlocale(LC_COLLATE, "");
        qsort(entries, entryCount, sizeof(char *), compareEntries);
    }

    for (size_t i = 0; i < entryCount; i++) {
        lstat(entries[i], &st);
        if (S_ISDIR(st.st_mode)) {
            if (strstr(flags, "d"))
                printf("d - %s\n", entries[i]);
            traverseDirectory(entries[i], flags);
        }
        if (S_ISREG(st.st_mode)) {
            if (strstr(flags, "f"))
                printf("f - %s\n", entries[i]);
        }
        if (S_ISLNK(st.st_mode)) {
            if (strstr(flags, "l"))
                printf("l - %s\n", entries[i]);
        }
        free(entries[i]);
    }
    free(entries);

    if (!entry) {
        if (errno) {
            fprintf(stderr, "Error: Unable to read directory '%s'\n", path);
            errno = 0;
        }
    }
    closedir(dir);
}
