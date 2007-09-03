# makefile ljcs
# 

CXX = g++
CXXFLAGS = -pthread -g -Iinclude
#CXXFLAGS = -pthread -o2 -DNDEBUG -Iinclude

# gtkmm & sourceviewmm
# 
CXXFLAGS += `pkg-config --cflags libgtksourceviewmm-1.0`
LJEDLIBS += `pkg-config --libs libgtksourceviewmm-1.0`

# python plugin engine
# 
PYFLAGS = -DENABLE_PYTHON_PLUGIN_ENGINE \
            -I/usr/include/python2.5 \
            -I/usr/include/pygtk-2.0
PYLIBS = -lpython2.5

LJEDOBJS = src/DocPageImpl.o             \
	src/DocManagerImpl.o             \
	src/CmdLineImpl.o                \
	src/CmdLineCallbacks.o           \
	src/LJEditorImpl.o               \
	src/LJEditorUtilsImpl.o          \
	src/MainWindowImpl.o             \
	src/PluginManager.o              \
	src/LJEditorPythonPluginEngine.o \
	src/main.o

BIN = bin/_ljedit

all : $(BIN) plugin

$(BIN) : $(LJEDOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(LJEDOBJS) -ldl $(LJEDLIBS) $(PYLIBS)

src/LJEditorPythonPluginEngine.o : src/LJEditorPythonPluginEngine.cpp src/LJEditorPythonPluginEngineImpl.cpp
	$(CXX) $(CXXFLAGS) $(PYFLAGS) -c -o $@ $<

plugin :
	cd plugins/ljed_ljcs;make
	cd plugins/ljed_hello;make
	cd plugins/ljed_indent;make

clean :
	rm -f $(BIN)
	rm -f src/*.o
	make clean_plugin

clean_plugin :
	cd plugins/ljed_ljcs;make clean
	cd plugins/ljed_hello;make clean
	cd plugins/ljed_indent;make clean


