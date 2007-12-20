# makefile ljedit
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

CXXFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2 -Wall
	OBJPATH = release
endif

# gtkmm & sourceviewmm
# 
CXXFLAGS += `pkg-config --cflags gtksourceviewmm-2.0`
LJEDLIBS += `pkg-config --libs gtksourceviewmm-2.0` `pkg-config --libs gthread-2.0`

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

BIN = bin/_ljedit

all : $(BIN) plugin

$(BIN) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -ldl $(LJEDLIBS) $(PYLIBS)

$(OBJPATH)/%.o : $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJPATH)/ConfigManagerImpl.o : $(SRCPATH)/ConfigManagerImpl.cpp
	$(CXX) $(CXXFLAGS) $(PYFLAGS) -c -o $@ $<

$(OBJPATH)/LJEditorPythonPluginEngine.o : $(SRCPATH)/LJEditorPythonPluginEngine.cpp
	$(CXX) $(CXXFLAGS) $(PYFLAGS) -c -o $@ $<

plugin :
	cd plugins/ljed_ljcs;make BUILD=$(BUILD)
	cd plugins/ljed_hello;make BUILD=$(BUILD)
	cd plugins/ljed_indent;make BUILD=$(BUILD)

clean :
	rm -f $(BIN)
	rm -rf debug
	rm -rf release
	make clean_plugin

clean_plugin :
	cd plugins/ljed_ljcs;make clean
	cd plugins/ljed_hello;make clean
	cd plugins/ljed_indent;make clean


