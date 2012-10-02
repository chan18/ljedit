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
else
	OUTPUT_SUFFIX = _gtk2
endif

TARGET_PATH = ../bin$(OUTPUT_SUFFIX)

ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

ifeq ($(MAKECMDGOALS), debug)
	COMMON_CFLAGS := -g -Wall -D_DEBUG
	OBJPATH = ../debug$(OUTPUT_SUFFIX)/puss
	TARGET = $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
	ifeq ($(OS), Windows_NT)
		TARGET_DLL = $(TARGET_PATH)/extends/puss_d.dll
	endif
else
	COMMON_CFLAGS := -O3 -Wall
	OBJPATH = ../release$(OUTPUT_SUFFIX)/puss
	TARGET = $(TARGET_PATH)/puss$(EXE_SUFFIX)
	ifeq ($(OS), Windows_NT)
		TARGET_DLL = $(TARGET_PATH)/extends/puss.dll
		COMMON_CFLAGS += -mwindows
	endif
endif

CFLAGS += $(COMMON_CFLAGS)

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

release : $(TARGET) $(TARGET_DLL) $(TARGET_PATH)/include/IPuss.h

debug : $(TARGET) $(TARGET_DLL) $(TARGET_PATH)/include/IPuss.h

clean :
	rm -rf ../debug$(OUTPUT_SUFFIX)/puss
	rm -rf ../release$(OUTPUT_SUFFIX)/puss
	rm -f $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
	rm -f $(TARGET_PATH)/puss$(EXE_SUFFIX)
	rm -f $(TARGET_PATH)/extends/puss_d.dll
	rm -f $(TARGET_PATH)/extends/puss.dll
	rm -f $(TARGET_PATH)/include/IPuss.h

ifeq ($(OS), Windows_NT)

$(TARGET) : $(OBJPATH)/win32/win32_main.o $(OBJPATH)/win32/puss_res.o
	$(CC) $(COMMON_CFLAGS) -o $@ $^

$(TARGET_DLL) : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

$(OBJPATH)/win32/win32_main.o : win32_main.c
	@mkdir -p $(OBJPATH)/win32
	$(CC) $(COMMON_CFLAGS) -D_USE_THIS_MODULE -o $@ -c $<

$(OBJPATH)/win32/puss_res.o : puss.rc puss.ico
	@mkdir -p $(OBJPATH)/win32
	$(WINDRES) -i puss.rc -J rc -o $@ -O coff --include-dir=./

else

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

endif

$(TARGET_PATH)/include/IPuss.h : IPuss.h
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

