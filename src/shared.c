#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#include <direct.h>
#endif

#ifdef __linux__
	#include <linux/limits.h>
	#define MAX_PATH PATH_MAX
#endif

#if defined(__unix__)||defined(__APPLE__)
	#define mkdir(dir) mkdir(dir, 0770)
#else
	#define mkdir(dir) _mkdir(dir)
#endif

#include <errno.h>

int xavaMkdir(char *dir) {
	/* Stolen from: https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950 */
	/* Adapted from http://stackoverflow.com/a/2336245/119527 */
	const size_t len = strlen(dir);
	char _path[MAX_PATH];
	char *p;

	errno = 0;

	/* Copy string so its mutable */
	if (len > sizeof(_path)-1) {
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(_path, dir);

	/* Iterate the string */
	for (p = _path + 1; *p; p++) {
		if (*p == '/') {
			/* Temporarily truncate */
			*p = '\0';

			if (mkdir(_path) != 0) {
				if (errno != EEXIST)
					return -1;
			}

			*p = '/';
		}
	}

	if (mkdir(_path) != 0) {
		if (errno != EEXIST)
			return -1;
	}
}

int xavaGetConfigDir(char *configPath) {
	#if defined(__unix__)||defined(__APPLE__)
		char *configFile = "config";
		char *configHome = getenv("XDG_CONFIG_HOME");
	#elif defined(WIN)
		// editing files without an extension on windows is a pain
		char *configFile = "config.cfg";
		char *configHome = getenv("APPDATA");
	#endif
	// don't worry, this will never happen on windows unless you are running like 98
	if (configHome != NULL) {
		#ifdef __WIN32__
			sprintf(configPath,"%s\\%s\\", configHome, PACKAGE);
		#else
			sprintf(configPath,"%s/%s/", configHome, PACKAGE);
		#endif
	} else {
		configHome = getenv("HOME");
		if (configHome != NULL)
			sprintf(configPath,"%s/%s/%s/", configHome, ".config", PACKAGE);
		else return -1;
	}
	return 0;
}

char *xavaGetInstallDir() {
	#ifdef __WIN32__
		// Windows uses widechars internally
		WCHAR wpath[MAX_PATH];
		char *path = malloc(MAX_PATH);

		// Get path of where the executable is installed
		HMODULE hModule = GetModuleHandleW(NULL);
		GetModuleFileNameW(hModule, wpath, MAX_PATH);
		wcstombs(path, wpath, MAX_PATH);

		// Hardcoded things pain me, but writing a 100 line-long
		// string replace function for windows is a no-no.
		// This is supposed to find and replace %PROGRAM_NAME%.exe 
		// at the end of the string
		size_t executableNameSize = strlen(PACKAGE".exe");
		path[strlen(path)-executableNameSize] = '\0';
	#else 
		// everything non-windows is simple as fuck, go look at the mess above
		const char *path = PREFIX"/share/"PACKAGE"/";
	#endif
	return path;
}

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

// size_t* is the filesize after reading
char* readTextFile(char *filename, size_t *size) {
	size_t k;

	FILE *fp = fopen(filename, "r");
	if(!fp) {
		fprintf(stderr, "%s is not readable/openable!\n", filename);
		return NULL;
	}

	// get the file size
	fseek(fp, 0, SEEK_END);
	k = ftell(fp);
	rewind(fp);

	// obligatory memory allocation
	char *content = malloc(k+1);
	if(!content) {
		fprintf(stderr, "!\n");
		return NULL;
	}

	// read the damn file
	fread(content, k, 1, fp);

	// clean your memory, kids
	fclose(fp);

	// don't return file size on an invalid pointer
	if(size!=NULL) (*size) = k;

	return content;
}

// both arguments are relative to the paths within the individual folders as detected in the host OS
_Bool loadDefaultConfigFile(char *origFile, char *destFile, char *destPath, char *subdir) {
	FILE *fp;
	if(xavaGetConfigDir(destPath)) {
		fprintf(stderr, "No HOME found (ERR_HOMELESS), exiting...");
		exit(EXIT_FAILURE);
	}

	strcat(destPath, subdir);

	if(subdir[0] != '\0') {
		#ifdef WIN
			strcat(destPath, "\\");
		#else
			strcat(destPath, "/");
		#endif
	}


	// config: create directory
	xavaMkdir(destPath);

	// config: adding default filename file
	strcat(destPath, destFile);

	fp = fopen(destPath, "r");
	if (!fp) {
		printf("Default %s doesn't exist in the current config folder. "
			"Going to try to create the file at the correct destination...",
			destFile);

		char *installPath = xavaGetInstallDir();
		// don't trust sizeof(), it's evil
		char *targetFile = malloc(strlen(installPath)+strlen(origFile)+strlen(subdir)+2);
		strcpy(targetFile, installPath);
		#ifdef __WIN32__
			// i hate when compilers force me into bullshit like this
			free(installPath);
		#endif
		strcat(targetFile, subdir);
		if(subdir[0] != '\0') {
			#ifdef WIN
				strcat(targetFile, "\\");
			#else
				strcat(targetFile, "/");
			#endif
		}
		strcat(targetFile, origFile);

		// because the program is not priviledged, read-only only
		FILE *source = fopen(targetFile, "r");

		if(!source) {
			fprintf(stderr, "FAIL\nDefault %s doesn't exist. "
				"Please provide one if you can!\n", targetFile);
			free(targetFile);
			return 1;
		} else {
			fp = fopen(destPath, "w");
			if(!fp) {
				fprintf(stderr, "FAIL\n Couldn't create %s! "
					"Please check if you have the appopriate permissions!\n",
					destPath);
				return 1;
			}

			// Copy file in the most C way possible
			short c = fgetc(source);
			while (c != EOF) {
				fputc(c, fp);
				c = fgetc(source);
			}
			fclose(source);
			fclose(fp);
			free(targetFile);
			printf("DONE\n");
		}
	}
	return 0;
}

