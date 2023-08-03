#include <stdio.h>
#include <stdint.h>
#include "ucurses.h"

void (*putnstr_func)(const char *, size_t) = NULL;
int (*getchar_func)(void) = NULL;

static void put_nstr(const char *str, size_t count);
static void put_num(uint16_t num);
static void put_csi();

enum {GM_NONE = 0, GM_ESC, GM_CSI, GM_CURSOR} getch_mode = GM_NONE;

void
set_putnstr_func(void (*_putnstr)(const char *, size_t))
{
  putnstr_func = _putnstr;
}

void
set_getchar_func(int (*_getchar)(void))
{
  getchar_func = _getchar;
}

void
initscr()
{
}

void
endwin()
{
}

void
move(int y, int x)
{
  put_csi();
  put_num(y + 1);
  put_nstr(";", 1);
  put_num(x + 1);
  put_nstr("H", 1);
}

void
save_cursor_position()
{
  char tmp[2] = {ESC, '7'};
  put_nstr(tmp, 2);
}

void
restore_cursor_position()
{
  char tmp[2] = {ESC, '8'};
  put_nstr(tmp, 2);
}

void
addstr(const char *str)
{
  int len = 0;
  const char *tmp = str;
  while (*tmp++) {
	len ++;
  }
  put_nstr(str, len);
}

void
addch(char ch)
{
  put_nstr(&ch, 1);
}

void
clear()
{
  put_csi();
  put_nstr("2J", 2);
}

void
clrtoeol()
{
  put_csi();
  put_nstr("0K", 2);
}

void
clrtobot()
{
  put_csi();
  put_nstr("0J", 2);
}

int
getch()
{
  int param1 = 0;

  if (getchar_func == NULL) {
	return 0;
  }
  getch_mode = GM_NONE;
  while (1) {
	int ch = (*getchar_func)();
	switch (getch_mode) {
	case GM_NONE:
	  if (ch == ESC) {
		getch_mode = GM_ESC;
	  } else {
		return ch;
	  }
	  break;
	case GM_ESC:
	  if (ch == '[') {
		getch_mode = GM_CSI;
	  } else if (ch == 'O') {
		getch_mode = GM_CURSOR;
	  } else if (ch == 'v' || ch == 'V') {
		return KEY_PPAGE;
	  } else if (ch == ESC) {
		return KEY_ESC;
	  } else if (ch == '<') {
		return KEY_SHOME;
	  } else if (ch == '>') {
		return KEY_SEND;
	  } else {
		return KEY_MAX;
	  }
	  break;
	case GM_CSI:
	  if (ch >= '0' && ch <='9') {
		param1 = (param1 * 10) + ch - '0';
		break;
	  } else if (ch == 'A') {
		return KEY_UP;
	  } else if (ch == 'B') {
		return KEY_DOWN;
	  } else if (ch == 'C') {
		return KEY_RIGHT;
	  } else if (ch == 'D') {
		return KEY_LEFT;
	  } else if (ch == 'F') {
		return KEY_END;
	  } else if (ch == 'H') {
		return KEY_HOME;
	  } else if (ch == '~') {
		if (param1 == 2) {
		  return KEY_IC;
		} else if (param1 == 3) {
		  return KEY_DC;
		} else if (param1 == 5) {
		  return KEY_PPAGE;
		} else if (param1 == 6) {
		  return KEY_NPAGE;
		} else if (param1 == 15) {
		  return KEY_F0 + 5 + param1 - 15;
		} else if (param1 >= 17 && param1 <= 21) {
		  return KEY_F0 + 6 + param1 - 17;
		}
		return KEY_MAX;
	  }
	  break;
	case GM_CURSOR:
	  if (ch >= 'P' && ch <= 'S') {
		return KEY_F0 + 1 + ch - 'P';
	  }
	  return KEY_MAX;
	}
  }
  return KEY_MAX;
}

/* Helper functions */
static void
put_nstr(const char *str, size_t count)
{
  if (putnstr_func == NULL) {
	return;
  }
  (*putnstr_func)(str, count);
}

static void
put_num(uint16_t num)
{
  uint16_t base = 1;
  while (base < 10000 && base * 10 <= num) {
	base *= 10;
  }
  while (base) {
	char ch = (num / base) + '0';
	put_nstr(&ch, 1);
	num %= base;
	base /= 10;
  }
}

static void
put_csi()
{
  char tmp[2] = {ESC, '['};
  put_nstr(tmp, 2);
}

//#define MAIN
#ifdef MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios oldt;

void
init_term()
{
  struct termios newt;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON|ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void
deinit_term()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void
main()
{
  set_putnstr_func(putnstr);
  put_num(1);
  put_char('\n');
  put_num(5);
  put_char('\n');
  put_num(9);
  put_char('\n');
  put_num(10);
  put_char('\n');
  put_num(99);
  put_char('\n');
  put_num(100);
  put_char('\n');
#if 0
  set_putchar_func(putchar);
  set_getchar_func(getchar);
  init_term();
  int ret = getch();
  printf("ret=%d\n", ret);
  deinit_term();
#endif
}
#endif /* MAIN */
