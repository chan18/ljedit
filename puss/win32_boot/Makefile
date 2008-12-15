# makefile win32_boot
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

ifeq ($(OS), Windows_NT)

TARGET = ../bin/puss.exe
all : $(TARGET)

$(TARGET) : main.c puss.rc puss.ico
	windres -i puss.rc -J rc -o puss_res.o -O coff --include-dir=./
	gcc -O3 -Wall -mwindows -o $@ $< puss_res.o

clean :
	@rm -f $(TARGET)

else

all :
clean :

endif
