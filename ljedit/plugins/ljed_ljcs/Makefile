# makefile ljcs
# 

CXX = g++
#CXXFLAGS = -pthread -g -DSHOW_PARSE_DEBUG_INFOS -fpic
CXXFLAGS = -pthread -o2 -DNDEBUG -fpic

GTKMMFLAGS = `pkg-config --cflags gtksourceviewmm-2.0` `pkg-config --cflags libglademm-2.4`
GTKMMLIBS = `pkg-config --libs gtksourceviewmm-2.0` `pkg-config --libs libglademm-2.4`
CXXFLAGS += $(GTKMMFLAGS)

LJEDINC = -I../../include
CXXFLAGS += $(LJEDINC)

LJCSINC = -I../../../ljcs/include
LJCSLIB = -L../../../ljcs/lib -lljcs
CXXFLAGS += $(LJCSINC)

OBJS = src/LJCSEnv.o      \
    src/LJCSIcons.o       \
    src/LJCSPlugin.o      \
    src/LJCSPluginImpl.o  \
    src/LJCSPluginUtils.o \
    src/ParseThread.o     \
    src/OutlinePage.o     \
    src/PreviewPage.o     \
    src/TipWindow.o       \
    src/SetupDialog.o

BIN = ../../bin/plugins/ljed_ljcs.so
all : $(BIN)

$(BIN) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(GTKMMLIBS) $(LJCSLIB) -shared -o $@ $(OBJS)
	cp ../../../ljcs/lib/libljcs.so ../../bin/plugins/

clean :
	rm -f $(BIN)
	rm -f src/*.o


