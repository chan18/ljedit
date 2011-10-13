# makefile
# 
# usage :
#     build debug version   : make debug
#     build release version : make
# 

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
endif

CC = gcc

TARGET_PATH = ../../bin

ifeq ($(MAKECMDGOALS), debug)
	CFLAGS = -g -Wall -D_DEBUG
	OBJPATH = debug
	TARGET = $(TARGET_PATH)/extends/pyextend.ext_d
else
	CFLAGS = -O3 -Wall
	OBJPATH = release
	TARGET = $(TARGET_PATH)/extends/pyextend.ext
endif

# gtk & gtksourceview & gthread & python & pygtk
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0` \

ifeq ($(PLATFORM), win32)
	CFLAGS += -IC:/Python27/include \
			-IC:/Python27/include/pygtk-2.0
	LIBS += -LC:/Python27/libs -lPython27

else
	CFLAGS += `pkg-config --cflags pygtk-2.0` \
		`pkg-config --cflags python`
	LIBS += `pkg-config --libs pygtk-2.0` \
		`pkg-config --libs python`
endif

# project
# 
CFLAGS += -I$(TARGET_PATH)/include

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)
DEPS = $(SRCS:%.c=$(OBJPATH)/%.d)
DIRS = $(sort $(foreach f,$(OBJS),$(dir $(f)).))

.PHONY : debug release clean

release : $(TARGET)

debug : $(TARGET)

clean :
	rm -f $(TARGET_PATH)/extends/pyextend.ext_d
	rm -f $(TARGET_PATH)/extends/pyextend.ext
	rm -rf debug
	rm -rf release

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(DIRS): % :
	@mkdir -p $@

$(OBJS): $(OBJPATH)/%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(DEPS): $(OBJPATH)/%.d : %.c | $(DIRS)
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif


