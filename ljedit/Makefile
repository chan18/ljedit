# makefile ljcs
# 

CXX = g++
CXXFLAGS = -pthread -g -DSHOW_PARSE_DEBUG_INFOS -Iinclude
#CXXFLAGS = -pthread -o2 -DNDEBUG -Iinclude

# gtkmm & sourceviewmm
# 
CXXFLAGS += `pkg-config --cflags libgtksourceviewmm-1.0`
LJEDLIBS += `pkg-config --libs libgtksourceviewmm-1.0`

# python plugin engine
# 
CXXFLAGS += -DENABLE_PYTHON_PLUGIN_ENGINE \
            -I/usr/include/python2.5 \
            -I/usr/include/pygtk-2.0
LJEDLIBS += -lpython2.5

LJEDOBJS = src/DocManagerImpl.o      \
	src/LanguageManager.o            \
	src/LJEditorImpl.o               \
	src/MainWindowImpl.o             \
	src/MiniCmdWindow.o              \
	src/PluginManager.o              \
	src/LJEditorPythonPluginEngine.o \
	src/main.o

BIN = bin/_ljedit

all : $(BIN)

$(BIN) : $(LJEDOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(LJEDOBJS) -ldl $(LJEDLIBS)

plugin :
	cd plugins/ljed_ljcs;make
	cd plugins/ljed_hello;make

clean :
	rm -f $(BIN)
	rm -f src/*.o

clean_plugin :
	cd plugins/ljed_ljcs;make clean
	cd plugins/ljed_hello;make clean


