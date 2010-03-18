# makefile
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
endif

TARGET_NAME = search_tools
TARGET_PATH = ../../bin/plugins

ifeq ($(BUILD), release)
	CXXFLAGS = -O3 -Wall
	OBJPATH = release
	TARGET = $(TARGET_PATH)/$(TARGET_NAME).so
else
	CXXFLAGS = -g -Wall -D_DEBUG
	OBJPATH = debug
	TARGET = $(TARGET_PATH)/$(TARGET_NAME)_d.so
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

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LIBS)

clean :
	rm -f $(TARGET)
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

