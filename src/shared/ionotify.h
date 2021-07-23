#ifndef __XAVA_SHARED_IONOTIFY_H
#define __XAVA_SHARED_IONOTIFY_H
typedef struct xava_ionotify* XAVAIONOTIFY;

// doubt that XAVA_IONOTIFY_DELETED ever be implemented, but I don't care
typedef enum xava_ionotify_event {
	XAVA_IONOTIFY_NOTHING,
	XAVA_IONOTIFY_ERROR,
	XAVA_IONOTIFY_CHANGED,
	XAVA_IONOTIFY_DELETED,
	XAVA_IONOTIFY_CLOSED
} XAVA_IONOTIFY_EVENT;

struct xava_ionotify_setup {
	const char *filename;
	void (*xava_ionotify_func)(XAVAIONOTIFY, XAVA_IONOTIFY_EVENT);
};

extern XAVAIONOTIFY xavaIONotifySetup(const struct xava_ionotify_setup *setup);
extern void         xavaIONotifyKill(const XAVAIONOTIFY ionotify);
#endif

