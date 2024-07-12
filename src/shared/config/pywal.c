#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shared/config/pywal.h"
#include "shared.h"

EXP_FUNC bool pywalConfigGetFilename(char *filename) {
char *a = getenv("XDG_CACHE_HOME");
    if (a == NULL){
        a = getenv("HOME");
    } else {
        sprintf(filename,"%s%s",a,"/wal/colors");
        return true;
    }
    if (a == NULL){
        return false;
    } else {
        sprintf(filename,"%s%s",a,"/.cache/wal/colors");
    }
    FILE *file = fopen(filename, "r");
    if ( file != NULL )
    {
        fclose(file);
    } else {
        return false;
    }
    return true;
}

EXP_FUNC void pywalGetColors(unsigned int *fgColorNum, unsigned int *bgColorNum) {
    int lineNumberFg = 1;
    int lineNumberBg = 2;
    char *filename = malloc(256);
    pywalConfigGetFilename(filename);
    FILE *file = fopen(filename, "r");
    int count = 0;
    if ( file != NULL )
    {
        char line[256]; /* or other suitable maximum line size */
        while (fgets(line, sizeof line, file) != NULL) /* read a line */
        {
            if (count == lineNumberFg)
            {
                sscanf(line, "#%06X",fgColorNum);
            }
            else if (count == lineNumberBg)
            {
                sscanf(line, "#%06X", bgColorNum);
                fclose(file);
                break;
            }
            else
            {
                count++;
            }
        }
    }
    free(filename);
}


    