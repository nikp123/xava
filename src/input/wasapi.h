#ifdef __cplusplus
	#define external extern "C"
#else
	#define external
#endif

external void *input_wasapi(void *audiodata);

#undef external
