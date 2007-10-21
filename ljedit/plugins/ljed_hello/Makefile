# makefile ljcs
# 

CXX = g++
#CXXFLAGS = -g -fpic
CXXFLAGS = -o2 -DNDEBUG -fpic

GTKMMFLAGS = `pkg-config --cflags gtksourceviewmm-2.0`
CXXFLAGS += $(GTKMMFLAGS)

LJEDINC = -I../../include
CXXFLAGS += $(LJEDINC)

GTKMMLIBS = `pkg-config --libs gtksourceviewmm-2.0`

OBJS = src/ljed_hello.o

BIN = ljed_hello.so

all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(GTKMMLIBS) -shared -o $@ $(OBJS)
	cp $@ ../../bin/plugins/

clean :
	rm -f ../../bin/plugins/$(BIN)
	rm -f $(BIN)
	rm -f src/*.o


