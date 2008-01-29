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

# python plugin engine
# 
PYFLAGS = -I/usr/include/python2.5 \
          -I/usr/include/pygtk-2.0
PYLIBS = -lpython2.5

# project
# 
CXXFLAGS += -Iinclude

SRCPATH = src

SRCS = $(wildcard $(SRCPATH)/*.cpp)

OBJS = $(SRCS:$(SRCPATH)/%.cpp=$(OBJPATH)/%.o)

TARGET = bin/_puss

all : $(TARGET) plugin

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -ldl $(LIBS) $(PYLIBS)

$(OBJPATH)/%.o : $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

#$(OBJPATH)/ConfigManagerImpl.o : $(SRCPATH)/ConfigManager.cpp
#	$(CXX) $(CXXFLAGS) $(PYFLAGS) -c -o $@ $<

#$(OBJPATH)/PythonPluginEngine.o : $(SRCPATH)/PythonPluginEngine.cpp
#	$(CXX) $(CCXXFLAGS) $(PYFLAGS) -c -o $@ $<

plugin :
	#cd plugins/hello;make BUILD=$(BUILD)

clean :
	rm -f $(TARGET)
	rm -rf debug
	rm -rf release
	#make clean_plugin

clean_plugin :
	cd plugins/hello;make clean

