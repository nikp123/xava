#include <stdbool.h>
#include <pthread.h>

#include "../ionotify.h"

typedef struct ionotify_specific_internal_DATA* IONOTIFYSPECIFICDATA;

struct xava_ionotify {
	const char           *filename;
	void                 (*xava_ionotify_func)(XAVAIONOTIFY, XAVA_IONOTIFY_EVENT);
	pthread_t            checker_thread;
	int                  pthread_id;
	bool                 alive;
	IONOTIFYSPECIFICDATA data;
};

void ioNotifyStop(const XAVAIONOTIFY inotify);
void *ioNotifySpecificWatchProcess(void *ionotify_ptr);

