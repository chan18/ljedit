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
SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

ifeq ($(PLATFORM), win32)
	TARGET = ../bin/_puss.exe

	ifeq ($(BUILD), release)
		LDFLAGS = -mwindows
	endif

else
	LIBS += -ldl
	TARGET = ../bin/puss

endif

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean :
	rm -f $(TARGET)
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : %.c
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:%.c=$(OBJPATH)/%.d)

