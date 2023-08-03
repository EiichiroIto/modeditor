# modeditor
Tiny text editor module for MicroPython.

## Installation

```
$ git clone --recursive https://github.com/micropython/micropython.git
$ git clone https://github.com/EiichiroIto/modeditor.git
```

## Build for Unix port

```
$ cd micropython/ports/unix/
$ make USER_C_MODULES=../../../modeditor/
$ build-standard/micropython
```

## Build for Rasberry Pi Pico port

```
$ cd micropython/ports/rp2/
$ make USER_C_MODULES=../../../modeditor/micropython.cmake
$ cp build-PICO/firmware.uf2 $(WHERE_PICO_MOUNTED)
```

## Usage
```
>>> import editor
>>> editor.edit("main.py")
```

### Save the file and Exit
Ctrl-X Ctrl-S

### Exit without saving
Ctrl-X Ctrl-C

## Options

### Buffer size

set buffer size to 4096 bytes. (defaults are 1024 bytes)

```
>>> dir(editor)
['__class__', '__name__', '__dict__', 'edit', 'set_buffer_size', 'set_screen', 'set_tab_width']
>>> editor.set_buffer_size(4096)
```

### Buffer size

set screen size to 80 cols by 24 rows. (defaults are 40 cols by 24 rows)

```
>>> editor.set_screen(80, 24)
```

### Tab width

set tab width to 8 characters. (defaults are 4)

```
>>> editor.set_tab_width(8)
```

