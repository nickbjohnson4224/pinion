#ifndef LOG_H
#define LOG_H

/*****************************************************************************
 * Kernel logging facilities
 *
 * This header provides an abstract interface to the logging facilities for
 * the kernel, independent of the actual storage/output medium for those logs.
 */

void __init_log(void);

void lograw(const char *fmt, ...);

void __log(int level, const char *func, int line, const char *fmt, ...);

#define log(level, ...) __log(level, __func__, __LINE__, __VA_ARGS__)

#define assert(expr) \
do {\
	if (!(expr)) {\
		log(DEBUG, "Derp! " #expr " is false!");\
	}\
} while (0);

#define assertfalse(expr) \
do {\
	if (expr) {\
		log(DEBUG, "Derp! " #expr " is true!");\
	}\
} while (0);

#define panic(msg) \
do {\
	log(ERROR, "I just don't know what went wrong!"); \
	log(ERROR, msg); \
	halt(); \
} while (0);

/* log levels ***************************************************************/

#define ERROR   0
#define INIT	1
#define DEBUG   2
#define VERBOSE 3

#define LOGLEVEL VERBOSE

#endif/*LOG_H*/
