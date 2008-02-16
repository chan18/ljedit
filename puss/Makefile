# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 
# locale :
#	xgettext -k_ -o puss.po src/*
#	cp puss.po puss.zh_CN.po
#	vi puss.zh_CN.po	# !!!not modify CHARSET to any-charset such as UTF-8
#	msgfmt -o bin/locale/zh_CN/LC_MESSAGES/puss.mo poss.zh_CN.po
# 

CXXFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2 -Wall
	OBJPATH = release
endif

# gtk & gtksourceview & gthread
# 
GTKINCS = `pkg-config --cflags gtksourceview-2.0`

GTKLIBS = `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
CXXFLAGS += -Iinclude

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:%.cpp=$(OBJPATH)/%.o)

TARGET = bin/puss

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -ldl $(LIBS) $(GTKLIBS)

clean :
	rm -f $(TARGET)
	rm -rf debug
	rm -rf release

$(OBJPATH)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTKINCS) -o $@ -c $<

$(OBJPATH)/%.d : %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@.tmp -MM $<	# use tmp file can ignore error!
	@echo -n $@ $(dir $@) > $@
	@cat $@.tmp >> $@
	@rm $@.tmp

-include $(SRCS:%.cpp=$(OBJPATH)/%.d)

