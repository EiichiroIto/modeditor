# modeditor
Tiny text editor module for MicroPython.

## Build for Unix port

```
$ cd micropython/ports/unix/
$ make USER_C_MODULES=$(WHERE_YOU_INSTALLED)/modeditor/
$ build-standard/micropython
```

## Build for Rasberry Pi Pico port

```
$ cd micropython/ports/rp2/
$ make USER_C_MODULES=$(WHERE_YOU_INSTALLED)/micropython.cmake
$ cp build-PICO/firmware.uf2 $(WHERE_PICO_MOUNTED)
```
