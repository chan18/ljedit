# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 
# locale example :
#	xgettext -k_ -o puss.po src/*
#	msgfmt -o bin/locale/zh_CN/LC_MESSAGES/puss.mo poss.po
# 

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
endif

CC = gcc
CFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CFLAGS = -O3 -Wall
	OBJPATH = release
endif

# gtk & gtksourceview & gthread
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gio-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
CFLAGS += -Iinclude

SRCS = $(wildcard src/*.c)

OBJS = $(SRCS:src/%.c=$(OBJPATH)/%.o)

ifeq ($(PLATFORM), win32)
	TARGET = bin/_puss.exe

	ifeq ($(BUILD), release)
		LDFLAGS = -mwindows
	endif

else
	LIBS += -ldl
	TARGET = bin/puss

endif

ifeq ($(PLATFORM), win32)
all : bin/puss.exe $(TARGET) puss_extends
else
all : $(TARGET) puss_extends
endif

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

bin/puss.exe : win32_boot/main.c win32_boot/puss.rc win32_boot/puss.ico
	windres -i win32_boot/puss.rc -J rc -o $(OBJPATH)/puss_res.o -O coff --include-dir=win32_boot
	$(CC) -O3 -Wall -mwindows -o $@ $< $(OBJPATH)/puss_res.o

deb :
	@./make_deb

zip :
	@rm -rf output
	@svn export ./bin output
	@if [ -f bin/_puss.exe ]; then cp -f bin/_puss.exe output/; fi
	@if [ -f bin/puss.exe ]; then cp -f bin/puss.exe output/; fi
	@if [ -f bin/puss ]; then cp -f bin/puss output/; fi
	@cp -rf bin/extends/*.ext output/extends/
	@echo tar jcf puss-1.0-`date +%F`.tar.bz2
	@cd output; tar jcf ../puss-1.0-`date +%F`.tar.bz2 *
	@rm -rf output

EXTS = $(wildcard extends/*)

puss_extends :
	@for ext in $(EXTS); do make -C$$ext BUILD=$(BUILD); done

clean :
	rm -f bin/puss.exe
	rm -f $(TARGET)
	rm -rf $(OBJPATH)

	@for ext in $(EXTS); do make -C$$ext clean BUILD=$(BUILD); done


$(OBJPATH)/%.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(GTKINCS) -o $@ -c $<

$(OBJPATH)/%.d : src/%.c
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:src/%.c=$(OBJPATH)/%.d)

