// Drops the last entry from the directory path
// "/example/path/1" -> "/example/path"
char *br_dirname(const char *path);

// Finds the full path of the current executable
char *find_exe(void);