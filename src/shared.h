// Audio sensitivity and volume varies greatly between 
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef XAVA_PREDEFINED_SENS_VALUE
	#define XAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);
