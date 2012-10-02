# makefile
# 
# usage :
#     build debug version   : make debug
#     build release version : make
# 

ifeq ($(GTK_VERSION),3)
	OUTPUT_SUFFIX = _gtk3
else
	OUTPUT_SUFFIX = _gtk2
endif

CC = gcc

TARGET_PATH = ../../bin$(OUTPUT_SUFFIX)

ifeq ($(MAKECMDGOALS), debug)
	CFLAGS = -g -Wall -D_DEBUG
	OBJPATH = ../../debug$(OUTPUT_SUFFIX)/python_ext
	TARGET = $(TARGET_PATH)/extends/pyextend.ext_d
else
	CFLAGS = -O3 -Wall
	OBJPATH = ../../release$(OUTPUT_SUFFIX)/python_ext
	TARGET = $(TARGET_PATH)/extends/pyextend.ext
endif

ifneq ($(OS), Windows_NT)
	CFLAGS += -fPIC
endif

# gtk & gtksourceview & gthread & python & pygtk
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0` \

ifeq ($(OS), Windows_NT)
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


