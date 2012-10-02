# makefile
# 
# usage :
#     build debug version   : make debug
#     build release version : make
#     build gtk3 version    : make GTK_VERSION=3
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
	OBJPATH = ../../debug$(OUTPUT_SUFFIX)/puss_controls_ext
	TARGET = $(TARGET_PATH)/extends/puss_controls.ext_d
else
	CFLAGS = -O3 -Wall
	OBJPATH = ../../release$(OUTPUT_SUFFIX)/puss_controls_ext
	TARGET = $(TARGET_PATH)/extends/puss_controls.ext
endif

ifneq ($(OS), Windows_NT)
	CFLAGS += -fPIC
endif

# gtk & gtksourceview
# 
ifeq ($(GTK_VERSION),3)
	CFLAGS += `pkg-config --cflags gtksourceview-3.0`
	LIBS += `pkg-config --libs gtksourceview-3.0`
else
	CFLAGS += `pkg-config --cflags gtksourceview-2.0`
	LIBS += `pkg-config --libs gtksourceview-2.0`
endif

# project
# 
CFLAGS += -I$(TARGET_PATH)/include

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)
DEPS = $(SRCS:%.c=$(OBJPATH)/%.d)
DIRS = $(sort $(foreach f,$(OBJS),$(dir $(f)).))

.PHONY : debug release clean

release : $(TARGET) $(TARGET_PATH)/include/IPussControls.h

debug : $(TARGET) $(TARGET_PATH)/include/IPussControls.h

clean :
	rm -f $(TARGET_PATH)/extends/puss_controls.ext_d
	rm -f $(TARGET_PATH)/extends/puss_controls.ext
	rm -f $(TARGET_PATH)/include/IPussControls.h
	rm -rf debug
	rm -rf release

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET_PATH)/include/IPussControls.h : IPussControls.h
	cp -f $< $@

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

