# makefile
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
endif

CFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CFLAGS = -O3 -Wall
	OBJPATH = release
endif

# gtk & gtksourceview(for libintl.h on win32)
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`
LIBS += `pkg-config --libs gtksourceview-2.0`

# project
# 
CFLAGS += -I../../bin/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET_NAME = gtk_doc_helper
TARGET_PATH = ../../bin/plugins
TARGET = $(TARGET_PATH)/$(TARGET_NAME)

all : $(TARGET).so $(TARGET).plugin

$(TARGET).so : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET).plugin : $(TARGET_NAME).plugin
	cp -f $< $@

clean :
	rm -f $(TARGET)
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : src/%.cpp
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:%.c=$(OBJPATH)/%.d)

