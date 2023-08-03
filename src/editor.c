#include <stdio.h>
#include <stdint.h>
#include "editor.h"

uint8_t *text = 0;
uint16_t maxtext, numtext;
uint16_t lines[MAX_ROWS];
uint8_t curx, cury;
uint16_t cursor;
uint8_t modified;
enum DrawMode drawmode;
uint8_t tabwidth = 4;
uint8_t rows = MAX_ROWS;

#define NOLINE			0xFFFF
#define SCROLL_ROWS		(rows/2)
#define IS_LFORNUL(c)   (((c)==LF)||((c)==NUL))

static void set_eof();
static void setup_lines(uint8_t start, uint16_t offset);
static int insert(uint16_t added);
static int delete(uint16_t removed);
static void insert_line(uint8_t line);
static void delete_line(uint8_t line);
static void update_lines(uint8_t line, uint8_t count, uint16_t offset);
static int scroll_up(uint8_t delta);
static int scroll_down(uint8_t delta);
static void move_bottom(uint16_t offset);
static uint16_t prevline(uint16_t offset);
static uint16_t nextline(uint16_t offset);
static uint8_t get_curx();
static uint16_t adjust_curx(uint8_t *org);

/* for initializing */

void
init_editor(uint8_t *_text, uint16_t max, uint8_t _rows)
{
  text = _text;
  maxtext = max - 2;
  rows = _rows > MAX_ROWS ? MAX_ROWS : _rows;
  import_start();
}

void
import_start()
{
  numtext = 0;
  lines[0] = 0;
  for (int i = 1; i < MAX_ROWS; i ++) {
    lines[i] = NOLINE;
  }
  curx = cury = 0;
  cursor = 0;
  set_eof();
  modified = 0;
  drawmode = DM_NONE;
}

int
import_data(const uint8_t *src, int size)
{
  uint8_t *dst = &text[numtext];
  int noerror = 1;

  while (size --) {
	uint8_t ch = *src++;

	if (!(ch == TAB || ch == LF || ch >= ' ')) {
	  continue;
	}
	if (numtext >= maxtext) {
	  noerror = 0;
	  break;
	}
	*dst++ = ch;
	numtext ++;
  }
  set_eof();
  return noerror;
}

void
import_end()
{
  setup_lines(0, 0);
  drawmode = DM_FULL;
}

/* for editing */

void
append_normalchar(uint8_t ch)
{
  if (!insert(1)) {
    return;
  }
  text[cursor ++] = ch;
  curx += get_charwidth(ch, curx);
  drawmode = DM_LINE;
}

void
append_newline()
{
  if (!insert(1)) {
    return;
  }
  drawmode = DM_BELOW;
  text[cursor ++] = LF;
  curx = 0;
  cury ++;
  if (cury >= rows) {
    cury -= scroll_down(SCROLL_ROWS);
  } else {
    insert_line(cury);
  }
  lines[cury] = cursor;
}

void
delete_char()
{
  uint8_t ch = text[cursor];
  if (!delete(1)) {
    return;
  }
  drawmode = DM_LINE;
  if (ch != LF) {
    return;
  }
  if (cury == rows - 1) {
    return;
  }
  drawmode = DM_BELOW;
  delete_line(cury + 1);
  lines[rows - 1] = nextline(lines[rows - 2]);
}

void
backspace_char()
{
  if (cursor == 0) {
    return;
  }
  uint8_t ch = text[cursor - 1];
  if (ch == LF && cury == 0) {
    cury += scroll_up(SCROLL_ROWS);
  }
  cursor --;
  if (!delete(1)) {
    return;
  }
  drawmode = DM_LINE;
  if (ch == LF) {
    cury --;
    update_lines(cury, rows - cury, lines[cury]);
	drawmode = DM_BELOW;
  }
  curx = get_curx();
}

void
kill_line()
{
  if (cursor >= numtext) {
    return;
  }
  uint16_t pos = cursor;
  if (text[cursor] == LF) {
	pos ++;
	delete_char();
	return;
  }
  while ((pos < numtext) && !IS_LFORNUL(text[pos])) {
	pos ++;
  }
  delete(pos - cursor);
  drawmode = DM_LINE;
}

/* for moving cursor */

void
move_left()
{
  if (cursor == 0) {
    return;
  }
  if (text[-- cursor] == LF) {
    if (cury == 0) {
      cury += scroll_up(SCROLL_ROWS);
    }
    cury --;
  }
  curx = get_curx();
}

void
move_right()
{
  if (cursor >= numtext) {
    return;
  }
  uint8_t ch = text[cursor ++];
  if (ch == LF) {
    if (cury == rows - 1) {
      cury -= scroll_down(SCROLL_ROWS);
    }
    cury ++;
    curx = 0;
  } else {
    curx += get_charwidth(ch, curx);
  }
}

void
move_up()
{
  if (lines[cury] == 0) {
    return;
  }
  if (cury == 0) {
    cury += scroll_up(SCROLL_ROWS);
  }
  cury --;
  cursor = adjust_curx(&curx);
}

void
move_down()
{
  uint16_t offset = nextline(lines[cury]);
  if (offset == NOLINE) {
    return;
  }
  if (cury == rows - 1) {
    cury -= scroll_down(SCROLL_ROWS);
  }
  cury ++;
  cursor = adjust_curx(&curx);
}

void
move_top_of_line()
{
  while ((cursor > 0) && (text[cursor-1] != LF)) {
    cursor --;
  }
  curx = 0;
}

void
move_end_of_line()
{
  while ((cursor < numtext) && !IS_LFORNUL(text[cursor])) {
    cursor ++;
  }
  curx = get_curx();
}

void
move_top_of_text()
{
  update_lines(0, rows, 0);
  curx = 0;
  cury = 0;
  cursor = 0;
  drawmode = DM_FULL;
}

void
move_end_of_text()
{
  cursor = numtext;
  move_top_of_line();
  move_bottom(cursor);
  move_end_of_line();
}

void
do_scroll_up()
{
  uint16_t offset = lines[min(SCROLL_CONTEXT_ROWS - 1,cury)];
  if (offset == NOLINE) {
	return;
  }
  move_bottom(offset);
}

void
do_scroll_down()
{
  uint16_t offset = lines[rows - SCROLL_CONTEXT_ROWS];
  if (offset == NOLINE) {
	return;
  }
  setup_lines(0, offset);
  curx = cury = 0;
  cursor = offset;
  drawmode = DM_FULL;
}

/* for query */

const uint8_t *
get_top_of_line(uint8_t y)
{
  uint16_t offset = lines[y];
  if (offset == NOLINE) {
    return NULL;
  }
  return &text[offset];
}

uint8_t
get_charwidth(uint8_t ch, uint8_t pos)
{
  if (ch == TAB) {
    return tabwidth - (pos % tabwidth);
  } else if (ch < ' ') {
    return 0;
  }
  return 1;
}

/* for debugging, to be removed */

void
print_status()
{
  printf("maxtext=%d, numtext=%d, curx=%d, cury=%d, cursor=%d\n",
         maxtext, numtext, curx, cury, cursor);
}

void
print_lines()
{
  for (int i = 0; i < rows; i ++) {
	uint16_t offset = lines[i];
	const uint8_t *src = (const uint8_t *) "";
	if (offset != NOLINE) {
	  src = &text[offset];
	}
    printf("%2d %d (%2.2s)\n", i, offset, src);
  }
}

/* support functions */

static void
set_eof()
{
  text[numtext] = NUL;
}

static void
setup_lines(uint8_t start, uint16_t offset)
{
  uint8_t count = rows - start;
  if (count == 0) {
    return;
  }
  while ((offset > 0) && (text[offset-1] != LF)) {
    offset --;
  }
  update_lines(start, count, offset);
}

static int
insert(uint16_t added)
{
  if ((added == 0) || ((numtext + added) >= maxtext)) {
    return 0;
  }
  modified = 1;
  uint16_t count = numtext - cursor;
  const uint8_t *src = &text[numtext-1];
  numtext += added;
  uint8_t *dst = &text[numtext-1];
  while (count --) {
    *dst-- = *src--;
  }
  set_eof();
  for (int i = cury + 1; i < rows; i ++) {
    if (lines[i] != NOLINE) {
      lines[i] += added;
    }
  }
  return 1;
}

static int
delete(uint16_t removed)
{
  if (cursor >= numtext) {
    return 0;
  }
  modified = 1;
  uint16_t count = numtext - cursor - removed;
  const uint8_t *src = &text[cursor + removed];
  uint8_t *dst = &text[cursor];
  while (count --) {
    *dst++ = *src++;
  }
  numtext -= removed;
  set_eof();
  for (int i = cury + 1; i < rows; i ++) {
    if (lines[i] != NOLINE) {
      lines[i] -= removed;
    }
  }
  return 1;
}

static void
insert_line(uint8_t line)
{
  uint8_t count = rows - line - 1;
  const uint16_t *src = &lines[rows - 2];
  uint16_t *dst = &lines[rows - 1];

  while (count --) {
    *dst-- = *src--;
  }
}

static void
delete_line(uint8_t line)
{
  uint8_t count = rows - line - 1;
  const uint16_t *src = &lines[line + 1];
  uint16_t *dst = &lines[line];

  while (count --) {
    *dst++ = *src++;
  }
}

static void
update_lines(uint8_t line, uint8_t count, uint16_t offset)
{
  uint16_t *dst = &lines[line];

  while (count --) {
    if (offset > numtext) {
      do {
        *dst++ = NOLINE;
      } while (count --);
      break;
    }
    *dst++ = offset;
    offset = nextline(offset);
  }
}

static int
scroll_up(uint8_t _delta)
{
  uint8_t delta;
  uint16_t offset = lines[0];
  for (delta = 0; delta < _delta; delta ++) {
    if (offset == 0) {
      break;
    }
    offset = prevline(offset);
  }
  if (delta == 0) {
    return 0;
  }
  uint8_t count = rows - delta;
  const uint16_t *src = &lines[rows - 1 - delta];
  uint16_t *dst = &lines[rows - 1];
  while (count --) {
    *dst-- = *src--;
  }
  update_lines(0, delta, offset);
  drawmode = DM_FULL;
  return delta;
}

static int
scroll_down(uint8_t delta)
{
  if (delta == 0) {
    return 0;
  }
  uint8_t count = rows - delta;
  const uint16_t *src = &lines[delta];
  uint16_t *dst = &lines[0];
  while (count --) {
    *dst++ = *src++;
  }
  uint16_t offset = nextline(*(src-1));
  update_lines(rows - delta, delta, offset);
  drawmode = DM_FULL;
  return delta;
}

static void
move_bottom(uint16_t offset)
{
  uint8_t count = rows - 1;
  while (count --) {
	offset = prevline(offset);
  }
  update_lines(0, rows, offset);
  curx = 0;
  cury = rows - 1;
  while (cury && lines[cury] == NOLINE) {
	cury --;
  }
  cursor = lines[cury];
  drawmode = DM_FULL;
}

static uint16_t
prevline(uint16_t offset)
{
  if (offset == 0) {
    return offset;
  }
  offset --;
  while ((offset > 0) && (text[offset-1] != LF)) {
    offset --;
  }
  return offset;
}

static uint16_t
nextline(uint16_t offset)
{
  if (offset == NOLINE) {
    return offset;
  }
  while (!IS_LFORNUL(text[offset])) {
    offset ++;
  }
  offset ++;
  if (offset > numtext) {
    return NOLINE;
  }
  return offset;
}

static uint8_t
get_curx()
{
  uint16_t top = lines[cury];
  uint16_t count = cursor - top;
  const uint8_t *src = &text[top];
  uint8_t pos = 0;

  while (count --) {
    pos += get_charwidth(*src++, pos);
  }
  return pos;
}

static uint16_t
adjust_curx(uint8_t *org)
{
  uint8_t pos = 0, ch, w;
  uint16_t offset = lines[cury];
  while ((offset < numtext) && ((ch = text[offset]) != LF)) {
    w = get_charwidth(ch, pos);
    if (pos + w > *org) {
      break;
    }
    pos += w;
    offset ++;
  }
  *org = pos;
  return offset;
}
