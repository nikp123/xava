#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../shared.h"

#ifdef __linux__
	#include <execinfo.h>
#endif

// internal stuff below

enum XAVA_MESSAGE_TYPE {
	XAVA_LOG_ERROR, XAVA_LOG_WARN, XAVA_LOG_NORM, XAVA_LOG_SPAM
};

static void __internal_xavaMsgHnd(enum XAVA_MESSAGE_TYPE mes, const char *fmt,
		const char *func, const char *file, int line, va_list list) {

	// please don't let this be a memory bug
	char newFmt[512];

	FILE *output = stdout;

	bool stack_trace = false;

	// process message headers
	switch(mes) {
		case XAVA_LOG_ERROR:
			snprintf(newFmt, 511, "[ERROR] %s at %s:%d - %s\n", func, file, line, fmt);
			output = stderr;
			stack_trace = true;
			break;
		case XAVA_LOG_WARN:
			snprintf(newFmt, 511, "[WARN] %s at %s:%d - %s\n", func, file, line, fmt);
			stack_trace = true;
			break;
		case XAVA_LOG_NORM:
			snprintf(newFmt, 511, "[INFO] %s at %s:%d - %s\n", func, file, line, fmt);
			break;
		case XAVA_LOG_SPAM:
			snprintf(newFmt, 511, "[SPAM] %s at %s:%d - %s\n", func, file, line, fmt);
			break;
	}

	vfprintf(output, newFmt, list);

	// add stack traces for better debugging
	if(stack_trace) {
	#ifdef __linux__
		void *stack_pointers[12];
		int stack_lenght = 12;
		char **stack_names;

		stack_lenght = backtrace(stack_pointers, stack_lenght);

		stack_names = backtrace_symbols(stack_pointers, stack_lenght);

		// skip the first two functions as they are part of the logging system itself
		for(int i=2; i<stack_lenght; i++) {
			printf("at %s\n", stack_names[i]);
		}
	#endif
	}
}


// global stuff below

EXP_FUNC void __internal_xavaSpam(const char *func, const char *file, int line, const char *fmt, ...) {
	va_list list;
	va_start(list, fmt);
	__internal_xavaMsgHnd(XAVA_LOG_SPAM, fmt, func, file, line, list);
	va_end(list);
}

EXP_FUNC void __internal_xavaLog(const char *func, const char *file, int line, const char *fmt, ...) {
	va_list list;
	va_start(list, fmt);
	__internal_xavaMsgHnd(XAVA_LOG_NORM, fmt, func, file, line, list);
	va_end(list);
}

EXP_FUNC void __internal_xavaWarn(const char *func, const char *file, int line, const char *fmt, ...) {
	va_list list;
	va_start(list, fmt);
	__internal_xavaMsgHnd(XAVA_LOG_WARN, fmt, func, file, line, list);
	va_end(list);
}

EXP_FUNC void __internal_xavaError(const char *func, const char *file, int line, const char *fmt, ...) {
	va_list list;
	va_start(list, fmt);
	__internal_xavaMsgHnd(XAVA_LOG_ERROR, fmt, func, file, line, list);
	va_end(list);
}

EXP_FUNC void __internal_xavaDie(void) {
	exit(EXIT_FAILURE);
}

