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
CFLAGS += `pkg-config --cflags gtk+-2.0`
LIBS += `pkg-config --libs gtk+-2.0`

# project
# 
CFLAGS += -I../../bin/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET_NAME = dndopen
TARGET_PATH = ../../bin/plugins
TARGET = $(TARGET_PATH)/$(TARGET_NAME)

all : $(TARGET).so $(TARGET).plugin

$(TARGET).so : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET).plugin : $(TARGET_NAME).plugin
	cp -f $< $@

clean :
	rm -f $(TARGET_PATH)/dndopen.so
	rm -f $(TARGET_PATH)/dndopen.plugin
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

