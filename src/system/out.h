#ifndef OUT_H
#define OUT_H
#include <stdint.h>
void outb(uint16_t addr, uint8_t byte);
void outw(uint16_t addr, uint16_t byte);
void outd(uint16_t addr, uint32_t byte);

#endif
