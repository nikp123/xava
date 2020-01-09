#include <sys/time.h>
#include <unistd.h>

#ifdef __WIN32__
	#include <windows.h>
#endif

unsigned long xavaSleep(unsigned long oldTime, int framerate) {
	unsigned long newTime = 0;
	if(framerate) {
	#ifdef WIN
		SYSTEMTIME time;
		GetSystemTime(&time);
		newTime = time.wSecond*1000+time.wMilliseconds;
		if(newTime-oldTime<1000/framerate&&newTime>oldTime)
			Sleep(1000/framerate-(newTime-oldTime));
		GetSystemTime(&time);
		return time.wSecond*1000+time.wMilliseconds;
	#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		newTime = tv.tv_sec*1000+tv.tv_usec/1000;
		if(oldTime+1000/framerate>newTime)
			usleep((1000/framerate+oldTime-newTime)*1000);
		gettimeofday(&tv, NULL);
		return tv.tv_sec*1000+tv.tv_usec/1000;
	#endif
	}
	#ifdef WIN
	Sleep(oldTime);
	#else
	usleep(oldTime*1000);
	#endif
	return 0;
}
