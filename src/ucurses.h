#ifndef __UCURSES_H
#define __UCURSES_H

#define NUL '\0'
#define ESC '\033'

#define KEY_ESC         033
#define KEY_DOWN        0402
#define KEY_UP          0403
#define KEY_LEFT        0404
#define KEY_RIGHT       0405
#define KEY_HOME        0406
#define KEY_BACKSPACE   0407
#define KEY_F0          0410
#define KEY_F(n)        (KEY_F0+(n))
#define KEY_DC          0512
#define KEY_IC          0513
#define KEY_END         0550
#define KEY_NPAGE       0522
#define KEY_PPAGE       0523
#define KEY_SEND        0602
#define KEY_SHOME       0607
#define KEY_MAX         0777

#ifdef __cplusplus
extern "C" {
#endif

  void set_putnstr_func(void (*)(const char *, size_t));
  void set_getchar_func(int (*)(void));
  void initscr();
  void endwin();
  void move(int y, int x);
  void addch(char ch);
  void addstr(const char *str);
  void clear();
  void clrtoeol();
  void clrtobot();
  int getch();

  void save_cursor_position();
  void restore_cursor_position();

#ifdef __cplusplus
};
#endif

#endif /* __UCURSES_H */
