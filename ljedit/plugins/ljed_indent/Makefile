# makefile ljcs
# 

CXX = g++
CXXFLAGS = -pthread -g -DSHOW_PARSE_DEBUG_INFOS -fpic
#CXXFLAGS = -pthread -o2 -DNDEBUG -fpic

GTKMMFLAGS = `pkg-config --cflags libgtksourceviewmm-1.0`
CXXFLAGS += $(GTKMMFLAGS)

LJEDINC = -I../../include
CXXFLAGS += $(LJEDINC)

GTKMMLIBS = `pkg-config --libs libgtksourceviewmm-1.0`

OBJS = src/ljed_indent.o

BIN = ljed_indent.so

all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(GTKMMLIBS) -shared -o $@ $(OBJS)
	cp $@ ../../bin/plugins/

clean :
	rm -f ../../bin/plugins/$(BIN)
	rm -f $(BIN)
	rm -f src/*.o


