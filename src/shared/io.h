#include <stdint.h>
#include <stdbool.h>

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
	XAVA_FILE_TYPE_CUSTOM_CONFIG,    // where modified configuration files are stored
	XAVA_FILE_TYPE_CACHE,            // where cached files are stored (shader cache)
	XAVA_FILE_TYPE_PACKAGE,          // where files that are part of the packages are stored (assets or binaries)
	XAVA_FILE_TYPE_NONE,             // this is an error
	XAVA_FILE_TYPE_CUSTOM_READ,      // custom file that can only be readable (must include full path)
	XAVA_FILE_TYPE_CUSTOM_WRITE      // custom file that can be both readable and writable (must include full path)
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

typedef struct XAVA_GRAHPICAL_EVENT_STACK {
	int pendingEvents;
	XG_EVENT *events;
} XG_EVENT_STACK;

// simulated event stack
extern void            pushXAVAEventStack    (XG_EVENT_STACK *stack, XG_EVENT event);
extern XG_EVENT        popXAVAEventStack     (XG_EVENT_STACK *stack);
extern XG_EVENT_STACK *newXAVAEventStack     ();
extern void            destroyXAVAEventStack (XG_EVENT_STACK *stack);
extern bool            pendingXAVAEventStack (XG_EVENT_STACK *stack);
extern bool            isEventPendingXAVA    (XG_EVENT_STACK *stack, XG_EVENT event);

// OS abstractions
extern           int xavaMkdir(const char *dir);
extern          bool xavaFindAndCheckFile(XF_TYPE type, const char *filename, char **actualPath);
extern unsigned long xavaSleep(unsigned long oldTime, int framerate);
extern unsigned long xavaGetTime(void);

// file/memory abstractions
extern RawData *xavaReadFile(const char *file);
extern void     xavaCloseFile(RawData *file);
extern void    *xavaDuplicateMemory(void *memory, size_t size);

