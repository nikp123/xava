#include "reload.h"

#ifdef __linux__
	#include <sys/types.h>
	#include <sys/inotify.h>
	#include <errno.h>
	#define EVENT_SIZE  (sizeof(struct inotify_event))
	#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef __linux__
	int fd, wd, lenght, i;
	char buffer[BUF_LEN];
	char *filename;
	pthread_t  p_thread;
	int thr_id;
	int alive = 0;
#endif

#ifdef __linux__
void watchFileProcess(void *name) {
	alive = 1;
	char *fname = (char*)name;
	filename = malloc(strlen(fname));
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
}
#endif

void watchFile(char *name) {
#ifdef __linux__
	thr_id = pthread_create(&p_thread, NULL, watchFileProcess,
	(void *)name);
#endif
}

int getFileStatus() {
#ifdef __linux__
	// using the current implementation we can determine if the process has died it must mean the file has been changed
	return !alive;
#endif
	
	// if the OS is not supported, just act as it is
	return 0;	
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
}
