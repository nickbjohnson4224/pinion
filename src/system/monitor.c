#include "monitor.h"
#include "out.h"

static uint16_t cursor = 0;
static uint16_t color = 0x0F00;

static void scroll()
{
   if(cursor >= SCRHEIGHT*80)
   {
       int i;
       for (i = 0; i < 24*SCRWIDTH; i++)
       {
           VIDMEM[i] = VIDMEM[i+SCRWIDTH];
       }

       for (; i < SCRHEIGHT*SCRWIDTH; i++)
       {
           VIDMEM[i] = 0x0F20;
       }
       cursor = SCRWIDTH*24;
   }
}

void move_cursor()
{
   outb(VIDCTLREG, CUR_HB);
   outb(VIDCURREG, cursor >> 8);
   outb(VIDCTLREG, CUR_LB);
   outb(VIDCURREG, cursor);
} 

void clear(void) {

	for (int i = 0; i < 80 * 25; i++) {
		VIDMEM[i] = 0x0F00;
	}

	cursor = 0;
	move_cursor();
}

void setcolor(uint8_t col) {
	color = col << 8;
}

void printc(char c) {

	if (c == '\n') 
		cursor = cursor - (cursor % 80) + 80;
	else if (c == '\t')
		cursor = cursor - (cursor % 8) + 8;
	else if (c >= ' ')
	{
		VIDMEM[cursor] = color | c;
		cursor++;
	}
	scroll();
	move_cursor();
}

uint64_t strtoul(const char *str)
{
	uint64_t tmp = 0;
	while(*str)
	{
		if(*str >= '0' && *str <= '9')
		{
			tmp *= 10;
			tmp += *str - '0';
			str++;
		}
		else
			break;
	}
	return tmp;
}

void escape(const char *str)
{
	int i;
	if(*str != '[')
		return;
	str++;
	i=0;
	while(str[i] != 'm')
	{
		switch(strtoul(str + i))
		{
			case 30:
				setcolor(COLOR(color >> 12, BLACK));
				break;
			case 31:
				setcolor(COLOR(color >> 12, RED));
				break;
			case 32:
				setcolor(COLOR(color >> 12, GREEN));
				break;
			case 33:
				break;
			case 34:
				setcolor(COLOR(color >> 12, BLUE));
				break;
			case 35:
				setcolor(COLOR(color >> 12, MAGENTA));
				break;
			case 36:
				setcolor(COLOR(color >> 12, CYAN));
				break;
			case 37:
				setcolor(COLOR(color >> 12, WHITE));
				break;
			case 40:
				setcolor(COLOR(BLACK, (color >> 8) & 0x0F));
				break;
			case 41:
				setcolor(COLOR(RED, (color >> 8) & 0x0F));
				break;
			case 42:
				setcolor(COLOR(GREEN, (color >> 8) & 0x0F));
				break;
			case 43:
				break;
			case 44:
				setcolor(COLOR(BLUE, (color >> 8) & 0x0F ));
				break;
			case 45:
				setcolor(COLOR(MAGENTA, (color >> 8) & 0x0F));
				break;
			case 46:
				setcolor(COLOR(CYAN, (color >> 8) & 0x0F));
				break;
			case 47:
				setcolor(COLOR(WHITE, (color >> 8) & 0x0F));
				break;
		}
		while(str[i] != 'm' && str[i] != ';') i++;
		if(str[i] == ';') i++;
	}
}

void prints(const char *string) {
	
	for (int i = 0;; i++) {

		if (string[i] == '\0') {
			break;
		}

		else if(string[i] == 0x1B) //escape sequence
		{
			escape(string + i + 1);
			while(string[++i] != 'm') ;
		}
		else
			printc(string[i]);
	}
}

void printx(unsigned int x) {
	char digit[16] = "0123456789ABCDEF";
	
	if (x == 0) {
		printc('0');
		return;
	}

	int i = 7;
	while (((x >> (4 * i)) & 0xF) == 0) i--;

	for (; i >= 0; i--) {
		printc(digit[(x >> (4 * i)) & 0xF]);
	}
}

void printd(int x) {
	char digit[10] = "0123456789";
	char buffer[10];

	if (x < 0) {
		printc('-');
		x = -x;
	}

	if (x == 0) {
		printc('0');
		return;
	}

	int i = 0;
	while (x) {
		buffer[i++] = digit[x%10];
		x /= 10;
	}

	while (i) {
		printc(buffer[--i]);
	}
}

void printp(uint32_t p) {
	char digit[16] = "0123456789ABCDEF";
	
	for (int i = 7; i >= 0; i--) {
		printc(digit[(p >> (4 * i)) & 0xF]);
	}
}
