#ifndef __XAVA_LOG_H
#define __XAVA_LOG_H

#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)

extern void __internal_xavaSpam (const char *func, char *file, int line, char *fmt, ...);
extern void __internal_xavaLog  (const char *func, char *file, int line, char *fmt, ...);
extern void __internal_xavaWarn (const char *func, char *file, int line, char *fmt, ...);
extern void __internal_xavaError(const char *func, char *file, int line, char *fmt, ...);
extern void __internal_xavaDie  (void);

// This is to shut up the code checker
#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#ifdef DEBUG
	#define xavaSpam(fmt, ...)  __internal_xavaSpam (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
	#define xavaLog(fmt, ...)   __internal_xavaLog  (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#else
	#define xavaSpam(fmt, ...)  /** nothing lol **/
	#define xavaLog(fmt, ...)   /** nothing lol **/
#endif

#define xavaWarn(fmt, ...)  __internal_xavaWarn (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#define xavaError(fmt, ...) __internal_xavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#define xavaBail(fmt, ...) { \
	__internal_xavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
	__internal_xavaDie(); \
}

#define xavaLogCondition(condition, fmt, ...) \
	if(condition) { __internal_xavaLog(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define xavaWarnCondition(condition, fmt, ...) \
	if(condition) { __internal_xavaWarn(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define xavaErrorCondition(condition, fmt, ...) \
	if(condition) { __internal_xavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define xavaBailCondition(condition, fmt, ...) { \
	if(condition) { \
		__internal_xavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
		__internal_xavaDie(); \
	} \
}

#endif

