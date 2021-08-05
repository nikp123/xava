#ifndef __XAVA_SHARED_IONOTIFY_H
#define __XAVA_SHARED_IONOTIFY_H

typedef struct xava_ionotify*             XAVAIONOTIFY;
typedef void*                             XAVAIONOTIFYWATCH;
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
	XAVAIONOTIFY      ionotify;
	int               id;
	char              *filename;
	void              (*xava_ionotify_func)(struct XAVA_HANDLE*, int id, XAVA_IONOTIFY_EVENT);
};

extern XAVAIONOTIFY      xavaIONotifySetup(struct XAVA_HANDLE *xava);
extern XAVAIONOTIFYWATCH xavaIONotifyAddWatch(XAVAIONOTIFYWATCHSETUP setup);
extern void              xavaIONotifyEndWatch(XAVAIONOTIFY handle, XAVAIONOTIFYWATCH watch);
extern void              xavaIONotifyKill(const XAVAIONOTIFY ionotify);
#endif

