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

# gtk & gtksourceview & gthread & python & pygtk
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0` \
	


ifeq ($(PLATFORM), win32)
	CFLAGS += -IC:/Python26/include \
			-IC:/Python26/include/pygtk-2.0

	LIBS += -LC:/Python26/libs -lpython26

else
	CFLAGS += `pkg-config --cflags pygtk-2.0` \
			-I/usr/include/python2.6

	LIBS += `pkg-config --libs pygtk-2.0` \
			-lpython2.6

endif

# project
# 
CFLAGS += -I../../bin/include

SRCS = $(wildcard *.c)

OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

TARGET = ../../bin/extends/pyextend.ext

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

ifneq ($(NOT_USE_D_FILE), 1)
-include $(SRCS:%.c=$(OBJPATH)/%.d)
endif

