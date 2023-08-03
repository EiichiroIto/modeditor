#include "py/runtime.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "extmod/vfs.h"
#include "editor.h"
#include "ucurses.h"
#include <string.h>

#define EDITOR_OFFSETX      0
#define EDITOR_OFFSETY      0

static uint16_t buffer_size = 1024;
static uint8_t *buffer = NULL;
static uint8_t editor_columns = 40;
static uint8_t editor_rows = 10;

STATIC mp_obj_t
set_screen(mp_obj_t width_obj, mp_obj_t height_obj)
{
  int size = mp_obj_get_int(width_obj);
  if (size > 0 && size < 256) {
    editor_columns = size;
  } else {
    mp_raise_ValueError(MP_ERROR_TEXT("Width values are exceeded."));
  }
  size = mp_obj_get_int(height_obj);
  if (size > 0 && size <= MAX_ROWS) {
    editor_rows = size;
  } else {
    mp_raise_ValueError(MP_ERROR_TEXT("Height values are exceeded."));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(set_screen_obj, set_screen);

STATIC mp_obj_t
set_buffer_size(mp_obj_t size_obj)
{
  int size = mp_obj_get_int(size_obj);
  if (size > 0 && size < 65535) {
    buffer_size = size;
  } else {
    mp_raise_ValueError(MP_ERROR_TEXT("size must be between 1 and 65534."));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(set_buffer_size_obj, set_buffer_size);

STATIC mp_obj_t
set_tab_width(mp_obj_t size_obj)
{
  int size = mp_obj_get_int(size_obj);
  if (size > 0 && size <= 8) {
    tabwidth = size;
  } else {
    mp_raise_ValueError(MP_ERROR_TEXT("tab width must be between 1 and 8."));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(set_tab_width_obj, set_tab_width);

static const char *
get_string(mp_obj_t obj, size_t *len)
{
  const char *str = NULL;
  if (mp_obj_is_str(obj)) {
    str = mp_obj_str_get_data(obj, len);
  }
  if (str == NULL) {
    mp_raise_TypeError(MP_ERROR_TEXT("can't convert 'int' object to str implicitly"));
  }
  return str;
}

static void
init_term()
{
#ifdef __linux__
  mp_hal_stdio_mode_raw();
#endif
  set_putnstr_func((void (*)(const char *, size_t))mp_hal_stdout_tx_strn);
  set_getchar_func((int (*)(void))mp_hal_stdin_rx_chr);
}

static void
deinit_term()
{
#ifdef __linux__
  mp_hal_stdio_mode_orig();
#endif
}

static void show_status()
{
  move(editor_rows+2, 0);
  clrtobot();
  print_status();
  print_lines();
}

static int showing_message = 0;

static void
show_message(const char *str)
{
  move(editor_rows, 0);
  if (str && *str) {
    addstr(str);
    showing_message = 1;
  } else {
    showing_message = 0;
  }
  clrtoeol();
}

static void
clear_message()
{
  if (showing_message) {
    show_message("");
  }
}

void
drawspaces(uint8_t count)
{
  while (count --) {
    addch(' ');
  }
}

void
drawline(uint8_t line)
{
  move(line + EDITOR_OFFSETY, EDITOR_OFFSETX);
  const uint8_t *src = get_top_of_line(line);
  if (src != NULL) {
    uint8_t pos = 0;
    while (pos < editor_columns) {
      uint8_t ch = *src++;
      uint8_t step = 1;
      if (ch == NUL || ch == LF) {
        break;
      } else if (ch == TAB) {
        step = get_charwidth(ch, pos);
        drawspaces(step);
      } else if (ch >= ' ') {
        addch(ch);
      }
      pos += step;
    }
  }
  clrtoeol();
}

void
drawall()
{
  for (int i = 0; i < editor_rows; i ++) {
    drawline(i);
  }
}

void
draw()
{
  if (drawmode == DM_FULL) {
    drawall();
  } if (drawmode == DM_BELOW) {
    drawall();
  } else if (drawmode == DM_LINE) {
    drawline(cury);
  }
  drawmode = DM_NONE;
  move(cury, min(curx,editor_columns-1));
}

#define CONTROL(key)    ((key)-'@')

#ifndef __linux__
extern int mp_interrupt_char;
#endif

int
editor_main()
{
  int to_be_saved = 0;
#ifndef __linux__
  int interrupt_char = mp_interrupt_char;
#endif

  while (1) {
    draw();
    int ch = getch();
    clear_message();
    if (ch == ESC) {
      break;
    } else if (ch == CONTROL('X')) {
      show_message("C-x- ");
      ch = getch();
      if (ch == CONTROL('C')) {
        break;
      } else if (ch == CONTROL('S')) {
        to_be_saved = 1;
        break;
      } else {
        show_message("");
      }
    } else if (ch == KEY_LEFT || ch == CONTROL('B')) {
      move_left();
    } else if (ch == KEY_RIGHT || ch == CONTROL('F')) {
      move_right();
    } else if (ch == KEY_UP || ch == CONTROL('P')) {
      move_up();
    } else if (ch == KEY_DOWN || ch == CONTROL('N')) {
      move_down();
    } else if (ch == KEY_HOME || ch == CONTROL('A')) {
      move_top_of_line();
    } else if (ch == KEY_END || ch == CONTROL('E')) {
      move_end_of_line();
    } else if (ch == KEY_SHOME) {
      move_top_of_text();
    } else if (ch == KEY_SEND) {
      move_end_of_text();
    } else if (ch == KEY_PPAGE) {
      do_scroll_up();
    } else if (ch == KEY_NPAGE || ch == CONTROL('V')) {
      do_scroll_down();
    } else if (ch == CONTROL('J') || ch == CONTROL('M')) {
      append_newline();
    } else if (ch == CONTROL('I')) {
      append_normalchar(ch);
    } else if (ch == KEY_DC || ch == CONTROL('D')) {
      delete_char();
    } else if (ch == KEY_BACKSPACE || ch == CONTROL('H')) {
      backspace_char();
    } else if (ch == CONTROL('K')) {
      kill_line();
    } else if (ch == CONTROL('G')) {
      drawmode = DM_FULL;
    } else if (ch == CONTROL('Q')) {
      show_status();
    } else if (ch >= ' ' && ch < 0x80) {
      append_normalchar(ch);
    } else {
      //printf("[%x]", ch);
    }
  }
#ifndef __linux__
  mp_interrupt_char = interrupt_char;
#endif
  return to_be_saved;
}

static void
read_file(const char *filename)
{
  mp_obj_t args[2] = {
    mp_obj_new_str(filename, strlen(filename)),
    MP_OBJ_NEW_QSTR(MP_QSTR_rb),
  };

  mp_obj_t file;
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0) {
    file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
    nlr_pop();
  } else {
    //printf("new file\n");
    return;
  }

  int errcode;
  byte buf[64];
  uint16_t len;
  do {
	len = mp_stream_rw(file, buf, sizeof buf, &errcode, MP_STREAM_RW_READ | MP_STREAM_RW_ONCE);
	if (errcode != 0) {
	  mp_raise_OSError(errcode);
	}
	if (len) {
	  if (!import_data((const uint8_t *) buf, len)) {
		show_message("*** Insufficient memory! ***");
		break;
	  }
	}
  } while (len);
  mp_stream_close(file);

  import_end();
  return;
}

static void
write_file(const char *filename, const uint8_t *buf, uint16_t size)
{
  mp_obj_t args[2] = {
    mp_obj_new_str(filename, strlen(filename)),
    MP_OBJ_NEW_QSTR(MP_QSTR_wb),
  };

  mp_obj_t file;
  file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

  //printf("mp_vfs_open done\n");

  int errcode;
  //uint16_t len = mp_stream_rw(file, (byte *) buf, size, &errcode, MP_STREAM_RW_WRITE | MP_STREAM_RW_ONCE);
  mp_stream_rw(file, (byte *) buf, size, &errcode, MP_STREAM_RW_WRITE | MP_STREAM_RW_ONCE);
  if (errcode != 0) {
    mp_raise_OSError(errcode);
  }

  //printf("write done=%d\n", len);
  
  mp_stream_close(file);
}

STATIC mp_obj_t
edit(mp_obj_t filename_obj)
{
  size_t filename_len = 0;
  const char *filename = get_string(filename_obj, &filename_len);
  if (filename_len == 0) {
    mp_raise_ValueError(MP_ERROR_TEXT("filename must not be empty."));
  }
  buffer = (uint8_t *) m_malloc(buffer_size);

  init_term();
  initscr();
  clear();
  move(0,0);
  init_editor(buffer, buffer_size, editor_rows);
  read_file(filename);
  if (editor_main()) {
	write_file(filename, buffer, strlen((const char*) buffer));
  }
  move(editor_rows, 0);
  endwin();
  deinit_term();

#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
  m_free(buffer, buffer_size);
#else /* MICROPY_MALLOC_USES_ALLOCATED_SIZE */
  m_free(buffer);
#endif /* MICROPY_MALLOC_USES_ALLOCATED_SIZE */
  buffer = NULL;
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(edit_obj, edit);

STATIC const mp_rom_map_elem_t example_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_editor) },
  { MP_ROM_QSTR(MP_QSTR_set_buffer_size), MP_ROM_PTR(&set_buffer_size_obj) },
  { MP_ROM_QSTR(MP_QSTR_set_screen), MP_ROM_PTR(&set_screen_obj) },
  { MP_ROM_QSTR(MP_QSTR_set_tab_width), MP_ROM_PTR(&set_tab_width_obj) },
  { MP_ROM_QSTR(MP_QSTR_edit), MP_ROM_PTR(&edit_obj) },
};
STATIC MP_DEFINE_CONST_DICT(example_module_globals, example_module_globals_table);

const mp_obj_module_t editor_module = {
  .base = { &mp_type_module },
  .globals = (mp_obj_dict_t *)&example_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_editor, editor_module);
