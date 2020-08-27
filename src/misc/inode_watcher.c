#include "inode_watcher.h"

#ifdef __linux__
	#include <sys/types.h>
	#include <sys/inotify.h>
	#include <errno.h>
	#include <unistd.h>
	#define EVENT_SIZE  (sizeof(struct inotify_event))
	#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#endif

#ifdef __WIN32__
	#include <windows.h>
	#include <tchar.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef __linux__
	int fd, wd, lenght, i;
	char buffer[BUF_LEN];
	char *filename;
#endif

#ifdef __WIN32__
	HANDLE dwFileChange;
	TCHAR *filename;
#endif

pthread_t  p_thread;
int thr_id;
int alive = 0;

#ifdef __linux__
void *watchFileProcess(void *name) {
	alive = 1;
	char *fname = (char*)name;
	filename = malloc(strlen(fname)+1);
	strcpy(filename, fname);
	fd = inotify_init();
	if(fd<0) {
		perror("inotify_init");
	}

	// we need to split the filename accordingly
	for(i=strlen(filename); i>0; i--) {
		if(filename[i-1]=='/') break;
	}
	filename[i-1] = 0x00;
	int separator = i;

	wd = inotify_add_watch(fd, filename, IN_MODIFY|IN_CREATE);
	char found = 0;
	while(!found) {
		lenght = read(fd, buffer, BUF_LEN);
		if(lenght < 0) {
			perror("read");
		}
		i=0;
		while(i<lenght) {
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			if(event->mask & IN_CREATE) if(!strcmp(event->name, &filename[separator])) found = 1;
			if(event->mask & IN_MODIFY) if(!strcmp(event->name, &filename[separator])) found = 1;
			i += EVENT_SIZE + event->len;
		}
	}
	free(filename);
	inotify_rm_watch(fd, wd);
	close(fd);
	alive = 0;
	return NULL;
}
#endif

#ifdef __WIN32__
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
#endif

void watchFile(char *name) {
	thr_id = pthread_create(&p_thread, NULL, watchFileProcess, (void *)name);
}

int getFileStatus() {
	// using the current implementation we can determine if the process has died it must mean the file has been changed
	return !alive;
}

void destroyFileWatcher() {
#ifdef __linux__
	// even though people don't like this, I'm going to do it like this anyway
	if(alive) {
		free(filename);
		pthread_cancel(p_thread);
		inotify_rm_watch(fd, wd);
		close(fd);
		alive = 0;
	}
#endif
#ifdef __WIN32__
	if(alive) {
		free(filename);
		pthread_cancel(p_thread);
		alive = 0;
	}
#endif
}
