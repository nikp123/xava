int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);
_Bool loadDefaultConfigFile(char *origFile, char *destFile, char *destPath, char *subdir);
char* readTextFile(char *filename, size_t *size);
