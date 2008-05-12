# makefile mlparser
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

CC = gcc
CFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CFLAGS = -O3 -Wall -DNDEBUG -DG_DISABLE_ASSERT
	OBJPATH = release
endif

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:%.c=$(OBJPATH)/%.o)

ifeq ($(BUILD), release)
	TARGET = lib/mlparser.a
else
	TARGET = lib/mlparser_d.a
endif

# glib
# 
CFLAGS += `pkg-config --cflags glib-2.0`
LIBS += `pkg-config --libs glib-2.0`

all : $(TARGET) test

$(TARGET) : $(OBJS)
	$(AR) -rc $@ $^

test : $(OBJS)
	$(CC) $(CFLAGS) -o tee $^ $(LIBS)

clean :
	rm -f $(TARGET)
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : %.cpp
	@echo $(CC) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:%.c=$(OBJPATH)/%.d)

