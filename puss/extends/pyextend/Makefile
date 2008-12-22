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

# gtk & gtksourceview & gthread & python & pygtk
# 
CXXFLAGS += `pkg-config --cflags gtksourceview-2.0`

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0` \
	


ifeq ($(PLATFORM), win32)
	CXXFLAGS += -IC:/Python25/include \
			-IC:/Python25/include/pygtk-2.0

	LIBS += -LC:/Python25/libs -lpython25

else
	CXXFLAGS += `pkg-config --cflags pygtk-2.0` \
			-I/usr/include/python2.5

	LIBS += `pkg-config --libs pygtk-2.0` \
			-lpython2.5

endif

# project
# 
CXXFLAGS += -I../../bin/include

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:src/%.cpp=$(OBJPATH)/%.o)

TARGET = ../../bin/extends/pyextend.ext

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

-include $(SRCS:src/%.cpp=$(OBJPATH)/%.d)

