#ifndef H_CONFIG
#define H_CONFIG
	// This is changed by the CMake build file, don't touch :)
	#ifndef XAVA_DEFAULT_INPUT
		#define XAVA_DEFAULT_INPUT "pulseaudio"
	#endif
	#ifndef XAVA_DEFAULT_OUTPUT
		#define XAVA_DEFAULT_OUTPUT "x11"
	#endif

	void load_config(char *configPath, void* p);
	void clean_config();
#endif
