# Project Path

## Structure
1. Add your header files (\*.h) to *inc*.
2. Add your source files (\*.c) to *src*.
3. Add external libraries you want to use to lib.


## Building & Flashing
### Adding files to Makefile
Add your files in the Makefile's "Add object files you need below" section. Remember to use the following syntax for clarity:
```bash
${BUILDPATH}/$(PROJ_NAME).axf: src/object_file_you_want_to_add.o
${BUILDPATH}/$(PROJ_NAME).axf: lib/src/external_library_object_file_you_want_to_add.o
```

### To build:
```bash
  make
```
### To flash:
```bash
  make flash
```
### To debug:
```bash
  make debug
```
### To clean project files:
```bash
  make clean
```
