#include <string.h>

#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>

#include "shared.h"

#include "../../shared.h"

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))

#define DIRBRK '/'

struct ionotify_specific_internal_DATA {
	int fd, wd;
	size_t filename_offset;
	char buffer[BUF_LEN];
	char *filename;
}; 

extern void *ioNotifySpecificWatchProcess(void *ionotify_ptr) {
	XAVAIONOTIFY         ionotify = ionotify_ptr;
	MALLOC_SELF(ionotify->data, 1); // because we STILL need to share pointers

	IONOTIFYSPECIFICDATA data     = ionotify->data;

	data->fd = inotify_init();
	if(data->fd == -1) {
		int errsv = errno;
		(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_ERROR);
		xavaReturnError(NULL, "inotify_init failed: %s", strerror(errsv));
	}

	// duplicate string
	data->filename = strdup(ionotify->filename);
	if(data->filename == NULL) {
		int errsv = errno;
		(*(ionotify->xava_ionotify_func))(ionotify, XAVA_IONOTIFY_ERROR);
		xavaReturnError(NULL, "strdup failed: %s", strerror(errsv));
	}

	// fix the filename
	for(int i=strlen(data->filename); i>0; i--) {
		if(data->filename[i] == DIRBRK) {
			data->filename[i] = 0x00;
			data->filename_offset = i+1;
		}
	}

	// add directory to file watch
	data->wd = inotify_add_watch(data->fd, data->filename, IN_MODIFY|IN_CREATE);
	if(data->wd == -1) {
		if(data->filename == NULL) {
			int errsv = errno;
			(*(ionotify->xava_ionotify_func))(ionotify, XAVA_IONOTIFY_ERROR);
			xavaReturnError(NULL, "ionotify_add_watch failed: %s", strerror(errsv));
		}
	}

	while(ionotify->alive) {
		size_t lenght = read(data->fd, data->buffer, BUF_LEN);
		xavaReturnErrorCondition(lenght<0, NULL, "read failed: %s", strerror(errno));

		size_t i = 0;
		while(i < lenght) {
			struct inotify_event *event = (struct inotify_event *) &data->buffer[i];

			if(event->mask & IN_IGNORED) {
				xavaLog("ionotify cancelled");
				(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_CLOSED);
				ionotify->alive = false;
				break;
			}

			bool filenames_match = false;
			if(!strcmp(event->name, &data->filename[data->filename_offset]))
				filenames_match = true;

			if((event->mask & IN_CREATE) && filenames_match)
				(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_CHANGED);
			if((event->mask & IN_MODIFY) && filenames_match)
				(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_CHANGED);

			i += EVENT_SIZE + event->len;
		}
	}

	close(data->wd);
	close(data->fd);
	free(data->filename);
	free(data);

	return NULL;
}

extern void ioNotifyStop(const XAVAIONOTIFY inotify) {
	IONOTIFYSPECIFICDATA data = inotify->data;
	inotify_rm_watch(data->fd, data->wd);
}

