#include <string.h>

#include "shared.h" 
#include "../../shared.h"

EXP_FUNC XAVAIONOTIFY xavaIONotifySetup(const struct xava_ionotify_setup *setup) {
	struct xava_ionotify *ionotify = MALLOC_SELF(ionotify, 1);

	char *realpath = strdup(setup->filename);
	xavaReturnErrorCondition(!xavaFindAndCheckFile(XAVA_FILE_TYPE_CUSTOM_READ,
				setup->filename, &realpath), NULL, "File %s not found", setup->filename); 
	ionotify->filename           = setup->filename;
	ionotify->xava_ionotify_func = setup->xava_ionotify_func;

	ionotify->pthread_id = pthread_create(&ionotify->checker_thread, NULL,
			ioNotifySpecificWatchProcess, ionotify);
	xavaReturnErrorCondition(ionotify->pthread_id, NULL,
			"pthread_create failed");

	ionotify->alive = true;

	return ionotify;
}

EXP_FUNC void xavaIONotifyKill(const XAVAIONOTIFY ionotify) {
	if(ionotify->alive) {
		ioNotifyStop(ionotify);
	}

	int error = pthread_join(ionotify->checker_thread, NULL);
	xavaErrorCondition(error, "pthread_join failed");

	// DO NOT DO THIS, NASTY EW BUG
	//free(ionotify->filename);
	free(ionotify);
}

