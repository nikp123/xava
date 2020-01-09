#define WASAPI_INPUT_NUM 7
#define WASAPI_INPUT_NAME "wasapi"

#ifdef __cplusplus
	#define external extern "C"
#else
	#define external
#endif

external void *input_wasapi(void *audiodata);

#undef external
