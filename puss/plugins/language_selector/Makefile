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
CFLAGS += -I../../bin/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET_NAME = language_selector
TARGET_PATH = ../../bin/plugins

TARGET = $(TARGET_PATH)/$(TARGET_NAME)

all : $(TARGET).so $(TARGET).plugin

$(TARGET).so : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET).plugin : $(TARGET_NAME).plugin
	cp -f $< $@

clean :
	rm -f $(TARGET).so
	rm -f $(TARGET).plugin
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

