# makefile puss
# 
# usage :
#     build debug version   : make debug
#     build release version : make
# 
# locale example :
#	xgettext -k_ -o puss.po ./*.c
#	msgfmt -o bin/locale/zh_CN/LC_MESSAGES/puss.mo poss.po
# 

CC = gcc

TARGET_PATH = ../bin

ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

ifeq ($(MAKECMDGOALS), debug)
	CFLAGS = -g -Wall -D_DEBUG
	OBJPATH = debug
	TARGET = $(TARGET_PATH)/puss_d$(EXE_SUFFIX)
else
	CFLAGS = -O3 -Wall
	OBJPATH = release
	TARGET = $(TARGET_PATH)/puss$(EXE_SUFFIX)
	ifeq ($(OS), Windows_NT)
		CFLAGS += -mwindows
	endif
endif

# gtk & gtksourceview & gthread
# 
CFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gio-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
define walk
	$(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

ALLFILES = $(call walk, .)

SRCS = $(filter %.c, $(ALLFILES))
OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)
DEPS = $(SRCS:%.c=$(OBJPATH)/%.d)
DIRS = $(sort $(foreach f,$(OBJS),$(dir $(f))))

.PHONY : debug release clean

release : $(TARGET) $(TARGET_PATH)/include/IPuss.h

debug : $(TARGET) $(TARGET_PATH)/include/IPuss.h

clean :
	rm -f $(TARGET)
	rm -f $(TARGET_PATH)/include/IPuss.h
	rm -rf $(OBJPATH)

$(TARGET) : $(OBJS) puss_res.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET_PATH)/include/IPuss.h : IPuss.h
	cp -f $< $@

puss_res.o : puss.rc puss.ico
	windres -i puss.rc -J rc -o puss_res.o -O coff --include-dir=./

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
