#ifndef __XAVA_SHARED_IONOTIFY_H
#define __XAVA_SHARED_IONOTIFY_H

#include <stdbool.h>

typedef void*                             XAVAIONOTIFY;
typedef struct xava_ionotify_watch_setup* XAVAIONOTIFYWATCHSETUP;

// make the compilers shuttings up
struct XAVA_HANDLE;

// doubt that XAVA_IONOTIFY_DELETED ever be implemented, but I don't care
typedef enum xava_ionotify_event {
    XAVA_IONOTIFY_NOTHING,
    XAVA_IONOTIFY_ERROR,
    XAVA_IONOTIFY_CHANGED,
    XAVA_IONOTIFY_DELETED,
    XAVA_IONOTIFY_CLOSED
} XAVA_IONOTIFY_EVENT;

struct xava_ionotify_watch_setup {
    XAVAIONOTIFY       ionotify;
    int                id;
    char               *filename;
    struct XAVA_HANDLE *xava;
    void               (*xava_ionotify_func)(XAVA_IONOTIFY_EVENT,
                                            const char *filename, 
                                            int id,
                                            struct XAVA_HANDLE*);
};

extern XAVAIONOTIFY      xavaIONotifySetup(void);
extern bool              xavaIONotifyAddWatch(XAVAIONOTIFYWATCHSETUP setup);
extern bool              xavaIONotifyStart(const XAVAIONOTIFY ionotify);
extern void              xavaIONotifyKill(const XAVAIONOTIFY ionotify);
#endif

