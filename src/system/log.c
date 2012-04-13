#include <stdint.h>
#include <stdarg.h>
#include "monitor.h"
#include "log.h"

/* logging interface ********************************************************/

static int _setup = 0;

void __init_log(void) {
	clear();
	_setup = 1;
}

void lograw(const char *fmt, ...) {
	va_list ap;
	
	if (!_setup) return;

	setcolor(L_BROWN);

	va_start(ap, fmt);
	for (int i = 0; fmt[i]; i++) {
		if (fmt[i] == '%') {
			i++;
			switch (fmt[i]) {
			case '\0': break;
			case '%': printc('%'); break;
			case 'd': printd(va_arg(ap, int)); break;
			case 'x': printx(va_arg(ap, unsigned int)); break;
			case 'p': printp(va_arg(ap, uint32_t)); break;
			case 's': prints(va_arg(ap, const char*)); break;
			case 'c': printc(va_arg(ap, int)); break;
			}
		}
		else {
			printc(fmt[i]);
		}
	}

	printc('\n');

	va_end(ap);
}

void __log(int level, const char *func, int line, const char *fmt, ...) {
	va_list ap;

	if (!_setup) return;

	if (level > LOGLEVEL) {
		return;
	}

	if (level != INIT) {
		setcolor(L_GREY);
		prints(func);
		prints(" ");
		printd(line);
		prints(":\t");
	}

	switch (level) {
	case ERROR:		setcolor(L_RED); break;
	case INIT:		setcolor(L_BROWN); break;
	case DEBUG:		setcolor(L_BLUE); break;
	case VERBOSE:	setcolor(L_GREY); break;
	default:		setcolor(BROWN); break;
	}

	va_start(ap, fmt);
	for (int i = 0; fmt[i]; i++) {
		if (fmt[i] == '%') {
			i++;
			switch (fmt[i]) {
			case '\0': break;
			case '%': printc('%'); break;
			case 'd': printd(va_arg(ap, int)); break;
			case 'x': printx(va_arg(ap, unsigned int)); break;
			case 'p': printp(va_arg(ap, uint32_t)); break;
			case 's': prints(va_arg(ap, const char*)); break;
			case 'c': printc(va_arg(ap, int)); break;
			}
		}
		else {
			printc(fmt[i]);
		}
	}

	va_end(ap);

	prints("\n");
	setcolor(L_GREY);
}
