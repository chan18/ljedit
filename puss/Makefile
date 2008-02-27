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
CXXFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0`

# project
# 
CXXFLAGS += -Iinclude

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:%.cpp=$(OBJPATH)/%.o)

TARGET = bin/puss

EXTENDS = 

all : $(TARGET) puss_extends

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -ldl $(LIBS)

puss_extends :
	@cd extends/ljcs;			make BUILD=$(BUILD)
	@cd extends/pyextend;		make BUILD=$(BUILD)
	@cd extends/plugin_engine;	make BUILD=$(BUILD)

clean :
	rm -f $(TARGET)
	rm -rf debug
	rm -rf release

	@cd extends/ljcs;			make clean
	@cd extends/pyextend;		make clean
	@cd extends/plugin_engine;	make clean

$(OBJPATH)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTKINCS) -o $@ -c $<

$(OBJPATH)/%.d : %.cpp
	@echo $(CXX) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@echo -n $@ $(dir $@) > $@.tmp
	@$(CXX) $(CXXFLAGS) -MM $< >> $@.tmp
	@mv $@.tmp $@

-include $(SRCS:%.cpp=$(OBJPATH)/%.d)

