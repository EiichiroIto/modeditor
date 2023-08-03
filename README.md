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
$ make USER_C_MODULES=../../../micropython.cmake
$ cp build-PICO/firmware.uf2 $(WHERE_PICO_MOUNTED)
```
