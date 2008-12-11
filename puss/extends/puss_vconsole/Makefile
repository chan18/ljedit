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

# gtk & gtksourceview & gthread
# 
PUSS_CFLAGS += `pkg-config --cflags gtksourceview-2.0`

PUSS_LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gio-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
PUSS_CFLAGS += -I../../include
PUSS_CFLAGS += -Ivconsole

SRCS = $(wildcard src/*.c)

OBJS = $(SRCS:src/%.c=$(OBJPATH)/%.o)

TARGET = ../../bin/extends/puss_vconsole.ext

VCONSOLE = ../../bin/extends/vconsole.dll

ifeq ($(PLATFORM), win32)

all : $(VCONSOLE) $(TARGET)

$(VCONSOLE) : vconsole/*.c
	$(CC) $(CFLAGS) -DUNICODE -shared -o $@ $^

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(PUSS_CFLAGS) -shared -o $@ $^ $(PUSS_LIBS)

clean :
	rm -f $(TARGET)
	rm -f $(VCONSOLE)
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PUSS_CFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : src/%.c
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(PUSS_CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:src/%.c=$(OBJPATH)/%.d)

else

all :
clean :

endif

