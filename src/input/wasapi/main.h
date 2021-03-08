// exported function, a macro used to determine which functions
// are exposed as symbols within the final library/obj files
#define EXP_FUNC __attribute__ ((visibility ("default")))

#ifdef __cplusplus
	#define external extern "C"
#else
	#define external
#endif
