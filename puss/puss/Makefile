# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 
# locale example :
#	xgettext -k_ -o puss.po ./*.c
#	msgfmt -o bin/locale/zh_CN/LC_MESSAGES/puss.mo poss.po
# 

ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

CC = gcc

TARGET_PATH = ../bin
ifeq ($(BUILD), release)
	CFLAGS = -O3 -Wall
	ifeq ($(OS), Windows_NT)
		CFLAGS += -mwindows
	endif
	OBJPATH = release
	TARGET = $(TARGET_PATH)/puss$(EXE_SUFFIX)
else
	CFLAGS = -g -Wall -D_DEBUG
	OBJPATH = debug
	TARGET = $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
	ifneq ($(OS), Windows_NT)
		LIBS += -ldl
	endif
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
SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

ifeq ($(OS), Windows_NT)
	OBJS += puss_res.o
endif

all : $(TARGET) $(TARGET_PATH)/include/IPuss.h

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET_PATH)/include/IPuss.h : IPuss.h
	cp -f $< $@

puss_res.o : puss.rc puss.ico
	windres -i puss.rc -J rc -o puss_res.o -O coff --include-dir=./

clean :
	rm -f $(TARGET)
	rm -f $(TARGET_PATH)/include/IPuss.h
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : %.c
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

ifneq ($(NOT_USE_D_FILE), 1)
-include $(SRCS:%.c=$(OBJPATH)/%.d)
endif
