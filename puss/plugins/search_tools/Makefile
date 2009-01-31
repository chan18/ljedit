# makefile
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
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
CXXFLAGS += -I../../bin/include

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:src/%.cpp=$(OBJPATH)/%.o)

TARGET_NAME = search_tools
TARGET_PATH = ../../bin/plugins

TARGET = $(TARGET_PATH)/$(TARGET_NAME)

all : $(TARGET).so $(TARGET).ui $(TARGET).plugin

$(TARGET).so : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LIBS)

$(TARGET).ui : $(TARGET_NAME).ui
	cp -f $< $@

$(TARGET).plugin : $(TARGET_NAME).plugin
	cp -f $< $@

clean :
	rm -f $(TARGET).so
	rm -f $(TARGET).ui
	rm -f $(TARGET).plugin
	rm -rf $(OBJPATH)

$(OBJPATH)/%.o : src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJPATH)/%.d : src/%.cpp
	@echo $(CXX) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MM -MT '$@ $(basename $@).o' $< -o $@

ifneq ($(NOT_USE_D_FILE), 1)
-include $(SRCS:src/%.cpp=$(OBJPATH)/%.d)
endif

