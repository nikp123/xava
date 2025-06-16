#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// define directory breaks cuz windows sucks
#ifdef __WIN32__
    #define DIRBRK '\\'
#else
    #define DIRBRK '/'
#endif

#ifdef __linux__
    // I've gotten this from a stranger on the internet
    #include <linux/limits.h>
    #define MAX_PATH PATH_MAX
#endif

#ifdef __WIN32__
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include <sys/syslimits.h>
    #define MAX_PATH PATH_MAX
#endif

#if defined(__unix__)||defined(__APPLE__)
    #define mkdir(dir) mkdir(dir, 0770)
#else
    #define mkdir(dir) mkdir(dir)
#endif

// XAVA file handlers
typedef enum XAVA_FILE_TYPE {
    XAVA_FILE_TYPE_CONFIG,           // where configuration files are stored
    XAVA_FILE_TYPE_OPTIONAL_CONFIG,  // where optional configuration files are stored
    XAVA_FILE_TYPE_CACHE,            // where cached files are stored (shader cache)
    XAVA_FILE_TYPE_PACKAGE,          // where files that are part of the packages are stored (assets or binaries)
    XAVA_FILE_TYPE_NONE,             // this is an error
} XF_TYPE;

// simulated data type
typedef struct data {
    size_t size;
    void*  data;
} RawData;

// XAVA event stuff
typedef enum XAVA_GRAHPICAL_EVENT {
    XAVA_REDRAW, XAVA_IGNORE, XAVA_RESIZE, XAVA_RELOAD,
    XAVA_QUIT
} XG_EVENT;

// simulated event stack
extern void      pushXAVAEventStack    (XG_EVENT *stack, XG_EVENT event);
extern XG_EVENT  popXAVAEventStack     (XG_EVENT *stack);
extern XG_EVENT *newXAVAEventStack     ();
extern void      destroyXAVAEventStack (XG_EVENT *stack);
extern bool      pendingXAVAEventStack (XG_EVENT *stack);
extern bool      isEventPendingXAVA    (XG_EVENT *stack, XG_EVENT event);

// OS abstractions
extern           int  xavaMkdir(const char *dir);
extern          char* xavaFindAndCheckFile(XF_TYPE type, const char *filename);
extern unsigned long  xavaSleep(unsigned long oldTime, int framerate);
extern unsigned long  xavaGetTime(void);
/**
 * A basic file copy function that takes in literal C string file paths as parameters
 * Both absolute and relative paths should work.
 *
 * @param source - The source file path
 * @param dest   - The destination file path
 */
extern bool xavaCopyFile(char *source, char *dest);

// file/memory abstractions
extern RawData *xavaReadFile(const char *file);
extern void     xavaCloseFile(RawData *file);
extern void    *xavaDuplicateMemory(void *memory, size_t size);

