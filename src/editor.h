#define MAX_ROWS			25
#define SCROLL_CONTEXT_ROWS 2

#define TAB					'\t'
#define LF					'\n'
#define NUL					'\0'

#define min(x,y)		((x)<(y)?(x):(y))

enum DrawMode {
  DM_NONE = 0, DM_LINE, DM_BELOW, DM_FULL
};

#ifdef __cplusplus
extern "C" {
#endif

  void init_editor(uint8_t *_text, uint16_t _max, uint8_t _rows);
  void import_start();
  int import_data(const uint8_t *src, int size);
  void import_end();

  void append_normalchar(uint8_t ch);
  void append_newline();
  void delete_char();
  void backspace_char();
  void kill_line();

  void move_left();
  void move_right();
  void move_up();
  void move_down();
  void move_top_of_line();
  void move_end_of_line();
  void move_top_of_text();
  void move_end_of_text();
  void do_scroll_up();
  void do_scroll_down();

  const uint8_t *get_top_of_line(uint8_t y);
  uint8_t get_charwidth(uint8_t ch, uint8_t pos);

  void print_status();
  void print_lines();

  extern uint16_t lines[];
  extern uint16_t numtext;
  extern uint8_t curx, cury;
  extern uint8_t modified;
  extern enum DrawMode drawmode;
  extern uint8_t tabwidth;
  extern uint8_t rows;

#ifdef __cplusplus
};
#endif
