#ifndef __XAVA_SHARED_IONOTIFY_H
#define __XAVA_SHARED_IONOTIFY_H

#include <stdbool.h>

// make the compilers shuttings up
typedef struct XAVA XAVA;

// doubt that XAVA_IONOTIFY_DELETED ever be implemented, but I don't care
typedef enum xava_ionotify_event {
  XAVA_IONOTIFY_NOTHING,
  XAVA_IONOTIFY_ERROR,
  XAVA_IONOTIFY_CHANGED,
  XAVA_IONOTIFY_DELETED,
  XAVA_IONOTIFY_CLOSED
} xava_ionotify_event;

/**
 * File watch handle
 *
 * DO NOT USE. Meant for internal use only
 **/
typedef struct xava_ionotify_file_handle {
  // The global XAVA instance
  XAVA *xava;

  // The desired event handler for said file
  void (*xava_ionotify_func)(xava_ionotify_event event, const char *filename,
                             int id, XAVA *);
} xava_ionotify_file_handle;

typedef struct xava_ionotify {
  // The global xwatcher instance
  void *xwatcher_instance;

  // List of handles corresponding to files that are being tracked
  // These hold the callback function pointers and the XAVA global instance
  xava_ionotify_file_handle *handles;
} xava_ionotify;

/**
 * Set up a specific file watcher using the created IONotify handle
 */
typedef struct xava_ionotify_watch_setup {
  xava_ionotify ionotify; // The global IONotify handle
  int id; // Optional ID that will be passed to your desired callback function
  char *filename; // The full path of the file that's going to be watched
  XAVA *xava;     // The global XAVA handle
  void (*xava_ionotify_func)(xava_ionotify_event, const char *filename, int id,
                             XAVA *); // The target callback fucntion
} xava_ionotify_watch_setup;

extern xava_ionotify xavaIONotifySetup(void);
extern bool xavaIONotifyAddWatch(xava_ionotify_watch_setup setup);
extern bool xavaIONotifyStart(const xava_ionotify ionotify);
extern void xavaIONotifyKill(const xava_ionotify ionotify);
#endif
