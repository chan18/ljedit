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

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
endif

CXXFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O3 -Wall
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

ifeq ($(PLATFORM), win32)
	TARGET = bin/_puss.exe

	ifeq ($(BUILD), release)
		LDFLAGS = -mwindows
	endif

else
	LIBS += -ldl
	TARGET = bin/puss

endif

ifeq ($(PLATFORM), win32)
all : bin/puss.exe $(TARGET) puss_extends
else
all : $(TARGET) puss_extends
endif

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

bin/puss.exe : win32_boot/main.c win32_boot/puss.rc win32_boot/puss.ico
	windres -i win32_boot/puss.rc -J rc -o $(OBJPATH)/puss_res.o -O coff --include-dir=win32_boot
	gcc -O3 -Wall -mwindows -o $@ $< $(OBJPATH)/puss_res.o

puss_extends :
	@cd extends/dndopen;		make BUILD=$(BUILD)
	@cd extends/ljcs;			make BUILD=$(BUILD)
	@cd extends/pyextend;		make BUILD=$(BUILD)
	@cd extends/plugin_engine;	make BUILD=$(BUILD)
	@cd extends/language_selector;	make BUILD=$(BUILD)

clean :
	rm -f $(TARGET)
	rm -f bin/puss.exe
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
	@$(CXX) $(CXXFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

-include $(SRCS:%.cpp=$(OBJPATH)/%.d)

