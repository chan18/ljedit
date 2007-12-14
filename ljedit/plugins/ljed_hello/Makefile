# makefile ljcs
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

CXXFLAGS = -g
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2 -DNDEBUG
	OBJPATH = release
endif

CXXFLAGS += -I../../include

# gtkmm & sourceviewmm
# 
CXXFLAGS += `pkg-config --cflags gtksourceviewmm-2.0`
GTKMMLIBS = `pkg-config --libs gtksourceviewmm-2.0`

BIN = ../../bin/plugins/ljed_hello.so

all : $(BIN)

$(BIN) : src/ljed_hello.cpp
	$(CXX) $(CXXFLAGS) $(GTKMMLIBS) -shared -o $@ $^

clean :
	rm -f $(BIN)
