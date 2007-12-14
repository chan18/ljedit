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

# gtkmm & sourceviewmm & glademm
# 
CXXFLAGS += `pkg-config --cflags gtksourceviewmm-2.0` `pkg-config --cflags libglademm-2.4`
GTKMMLIBS = `pkg-config --libs gtksourceviewmm-2.0` `pkg-config --libs libglademm-2.4`

# ljcs
# 
CXXFLAGS += -I../../../ljcs/include
LJCSLIB = -L../../../ljcs/lib -lljcs

SRCPATH = src

SRCS = $(wildcard $(SRCPATH)/*.cpp)

OBJS = $(SRCS:$(SRCPATH)/%.cpp=$(OBJPATH)/%.o)

BIN = ../../bin/plugins/ljed_ljcs.so

all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(GTKMMLIBS) $(LJCSLIB) -shared -o $@ $(OBJS)
	cp ../../../ljcs/lib/libljcs.so ../../bin/plugins/

$(OBJPATH)/%.o : $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean :
	rm -rf debug
	rm -rf release
	rm -f $(BIN)
