#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#define COLOR(bg,fg) ((((bg) & 0xF) << 4) | ((fg) & 0xF))
#define OFFSET(y,x) ((y)*80 + (x))
#define VIDCTLREG 0x3D4
#define VIDCURREG 0x3D5
#define VIDMEM ((uint16_t*) 0xF00B8000)
#define SCRWIDTH 80
#define SCRHEIGHT 25
#define CUR_HB 14
#define CUR_LB 15

enum {BLACK,BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, 
	L_GREY, D_GREY, L_BLUE, L_GREEN, L_CYAN, L_RED, L_MAGENTA, L_BROWN, WHITE};

void monitor_put(uint8_t c);
void monitor_clear();
void monitor_write(uint8_t *c);
void clear();
void setcolor(uint8_t color);
void printc(char c);
void prints(const char *string);
void printx(unsigned int x);
void printd(int x);
void printp(uint32_t p);

#endif
