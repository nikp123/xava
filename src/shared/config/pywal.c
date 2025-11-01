#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shared/config/pywal.h"
#include "shared.h"

// FIXME: This implementation really doesn't fit the project. FIX IT!

EXP_FUNC bool pywalConfigGetFilename(char *filename) {
    char *a = getenv("XDG_CACHE_HOME");
    if(a == NULL) {
        a = getenv("HOME");
        xavaBailCondition(a == NULL, "User is $HOME-less. Aborting.");

        sprintf(filename,"%s%s", a, "/.cache/wal/colors");
    } else {
        sprintf(filename,"%s%s", a, "/wal/colors");
    }

    FILE *file = fopen(filename, "r");

    if(file != NULL) {
        fclose(file);
    } else {
        return false;
    }

    return true;
}

#define PYWAL_MAX_LINE_LENGHT 256

EXP_FUNC void pywalGetColors(
        unsigned int *fgColorNum,
        unsigned int *bgColorNum
        ) {
    int lineNumberFg = 1;
    int lineNumberBg = 2;
    char *filename = malloc(MAX_PATH);
    pywalConfigGetFilename(filename);
    FILE *file = fopen(filename, "r");
    int count = 0;
    if(file != NULL) {
        char line[PYWAL_MAX_LINE_LENGHT]; /* or other suitable maximum line size */
        while (fgets(line, sizeof line, file) != NULL) {
            if (count == lineNumberFg) {
                sscanf(line, "#%06X", fgColorNum);
            } else if (count == lineNumberBg) {
                sscanf(line, "#%06X", bgColorNum);
                break;
            } else count++;
        }
        fclose(file);
    }
    free(filename);
}



