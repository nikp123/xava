#ifndef H_CONFIG
#define H_CONFIG
	// This is changed by the CMake build file, don't touch :)
	#ifndef XAVA_DEFAULT_INPUT
		#define XAVA_DEFAULT_INPUT "pulseaudio"
	#endif
	#ifndef XAVA_DEFAULT_OUTPUT
		#define XAVA_DEFAULT_OUTPUT "x11"
	#endif
	#ifndef XAVA_DEFAULT_FILTER
		#define XAVA_DEFAULT_FILTER "default"
	#endif

	#include "shared.h"

	char *load_config(char *configPath, struct XAVA_HANDLE*);
	void clean_config();
#endif
