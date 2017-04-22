#ifndef HTTPD_LOGGING_H
#define HTTPD_LOGGING_H

#include "uptime.h"

#ifndef VERBOSE_LOGGING
#define VERBOSE_LOGGING 0
#endif

#ifndef LOG_EOL
#define LOG_EOL "\n"
#endif
#define httpd_printf(fmt, ...) rtl_printf(fmt, ##__VA_ARGS__)

/**
 * Print a startup banner message (printf syntax)
 * Uses bright green color
 */
#define banner(fmt, ...) \
	do {  \
		httpd_printf(LOG_EOL "\x1b[32;1m");  \
		uptime_print();  \
		httpd_printf(" [i] " fmt "\x1b[0m" LOG_EOL LOG_EOL, ##__VA_ARGS__);  \
	} while(0)

/**
 * Same as 'info()', but enabled even if verbose logging is disabled.
 * This can be used to print version etc under the banner.
 */
#define banner_info(fmt, ...) \
	do { \
		httpd_printf("\x1b[32m"); \
		uptime_print(); \
		httpd_printf(" [i] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

#if VERBOSE_LOGGING
	/**
	 * Print a debug log message (printf format)
	 */
	#define dbg(fmt, ...) \
		do { \
			uptime_print(); \
			httpd_printf(" [ ] " fmt LOG_EOL, ##__VA_ARGS__); \
		} while(0)

	/**
	 * Print a info log message (printf format)
	 * Uses bright green color
	 */
	#define info(fmt, ...) \
		do { \
			httpd_printf("\x1b[32m"); \
			uptime_print(); \
			httpd_printf(" [i] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
		} while(0)
#else
	#define dbg(fmt, ...)
	#define info(fmt, ...)
#endif

/**
 * Print a error log message (printf format)
 * Uses bright red color
 */
#define httpd_error(fmt, ...) \
	do { \
		httpd_printf("\x1b[31;1m"); \
		uptime_print(); \
		httpd_printf(" [E] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

/**
 * Print a warning log message (printf format)
 * Uses bright yellow color
 */
#define warn(fmt, ...) \
	do { \
		httpd_printf("\x1b[33;1m"); \
		uptime_print(); \
		httpd_printf(" [W] " fmt "\x1b[0m"LOG_EOL, ##__VA_ARGS__); \
	} while(0)

#endif // HTTPD_LOGGING_H
