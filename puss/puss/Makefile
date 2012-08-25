# makefile puss
# 
# usage :
#     build debug version   : make debug
#     build release version : make
#     build gtk3 version    : make GTK_VERSION=3
# 
# locale example :
#	xgettext -k_ -o puss.po ./*.c
#	msgfmt -o bin/locale/zh_CN/LC_MESSAGES/puss.mo poss.po
# 

CC = gcc
WINDRES = windres

ifeq ($(GTK_VERSION),3)
	OUTPUT_SUFFIX = _gtk3
endif

TARGET_PATH = ../bin$(OUTPUT_SUFFIX)

ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
	WIN_RES = puss_res.o
endif

ifeq ($(MAKECMDGOALS), debug)
	CFLAGS = -g -Wall -D_DEBUG
	OBJPATH = ../debug$(OUTPUT_SUFFIX)/puss
	TARGET = $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
else
	CFLAGS = -O3 -Wall
	OBJPATH = ../release$(OUTPUT_SUFFIX)/puss
	TARGET = $(TARGET_PATH)/puss$(EXE_SUFFIX)
	ifeq ($(OS), Windows_NT)
		CFLAGS += -mwindows
	endif
endif

# gtk & gtksourceview & gthread
# 
ifeq ($(GTK_VERSION),3)
	CFLAGS += `pkg-config --cflags gtksourceview-3.0`
	LIBS += `pkg-config --libs gtksourceview-3.0`
else
	CFLAGS += `pkg-config --cflags gtksourceview-2.0`
	LIBS += `pkg-config --libs gtksourceview-2.0`
endif

LIBS += `pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gio-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
define walk
	$(patsubst ./%, %, $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e))))
endef

ALLFILES = $(call walk, .)

SRCS = $(filter %.c, $(ALLFILES))
OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)
DEPS = $(SRCS:%.c=$(OBJPATH)/%.d)
DIRS = $(sort $(foreach f,$(OBJS),$(dir $(f)).))	# why need release/. not release ??? in linux

.PHONY : debug release clean

release : $(TARGET) $(TARGET_PATH)/include/IPuss.h

debug : $(TARGET) $(TARGET_PATH)/include/IPuss.h

clean :
	rm -rf debug
	rm -rf release
	rm -f $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
	rm -f $(TARGET_PATH)/puss$(EXE_SUFFIX)
	rm -f $(TARGET_PATH)/include/IPuss.h

$(TARGET) : $(OBJS) $(WIN_RES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET_PATH)/include/IPuss.h : IPuss.h
	cp -f $< $@

puss_res.o : puss.rc puss.ico
	$(WINDRES) -i puss.rc -J rc -o puss_res.o -O coff --include-dir=./

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

