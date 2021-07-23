#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <windows.h>
#include <tchar.h>

#include "shared.h"

#include "../../shared.h"

#define DIRBRK '\\'

struct ionotify_specific_internal_DATA {
	HANDLE dwFileChange;
	TCHAR *filename;
	bool keep_alive;
};


void *ioNotifySpecificWatchProcess(void *ionotify_ptr) {
	XAVAIONOTIFY         ionotify = ionotify_ptr;
	MALLOC_SELF(ionotify->data, 1); // because we STILL need to share pointers

	IONOTIFYSPECIFICDATA data     = ionotify->data;

	data->filename = strdup(ionotify->filename);
	data->keep_alive = true;

	// remove filename from config file path
	for(int i=strlen(data->filename)-1; i>0; i--) {
		if(data->filename[i] == DIRBRK) {
			data->filename[i] = '\0';
			break;
		}
	}

	data->dwFileChange = FindFirstChangeNotification(data->filename, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	xavaBailCondition(data->dwFileChange == INVALID_HANDLE_VALUE, "FindFirstChangeNotification failed");
	while(data->keep_alive) {
		DWORD dwWaitStatus = WaitForSingleObject(data->dwFileChange, 100); // wait for 100ms
		switch(dwWaitStatus) {
			case WAIT_OBJECT_0:
				(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_CHANGED);
				break;
			case WAIT_ABANDONED:
			case WAIT_FAILED:
				(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_ERROR);
				break;
			case WAIT_TIMEOUT:
				// this is used to keep in sync with keep_alive
				break;
		}
	}
	(*ionotify->xava_ionotify_func)(ionotify, XAVA_IONOTIFY_CLOSED);

	free(data->filename);
	free(data);

	ionotify->alive = false;

	return NULL;
}

void ioNotifyStop(const XAVAIONOTIFY ionotify) {
	ionotify->data->keep_alive = false;
}
