# makefile
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
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

# gtk
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`
LIBS += `pkg-config --libs gtksourceview-2.0`

# project
# 
CFLAGS += -I../../bin/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET = ../../bin/extends/miniline.ext

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

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

