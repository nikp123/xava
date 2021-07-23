#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <windows.h>
#include <tchar.h>

HANDLE dwFileChange;
TCHAR *filename;

void *watchFileProcess(void *name) {
	filename = malloc(strlen((char*)name)+1);
	strcpy(filename, (char*)name);

	// remove filename from config file path
	for(int i=strlen(name)-1; i>0; i--) {
		if(filename[i] == '\\') {
			filename[i] = '\0';
			break;
		}
	}

	char found = 0;
	alive = 1;

	dwFileChange = FindFirstChangeNotification(filename, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	while(!found) {
		DWORD dwWaitStatus = WaitForSingleObject(dwFileChange, INFINITE);
		switch(dwWaitStatus) {
			case WAIT_OBJECT_0:
				found = 1;
				break;
		}
	}
	free(name);
	alive = 0;
	return NULL;
}


void destroyFileWatcher() {
	if(alive) {
		free(filename);
		pthread_cancel(p_thread);
		alive = 0;
	}
}
