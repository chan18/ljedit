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

# gtk & gtksourceview
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0`

# project
# 
TARGET_PATH = ../../bin

CFLAGS += -I$(TARGET_PATH)/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET = $(TARGET_PATH)/extends/puss_controls.ext

all : $(TARGET) $(TARGET_PATH)/include/IPussControls.h

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET_PATH)/include/IPussControls.h : IPussControls.h
	cp -f $< $@

clean :
	rm -f $(TARGET)
	rm -f $(TARGET_PATH)/include/IPussControls.h
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

