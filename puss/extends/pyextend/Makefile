# makefile
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

CXXFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2 -Wall
	OBJPATH = release
endif

# gtk & gtksourceview & gthread & python & pygtk
# 
CXXFLAGS += `pkg-config --cflags gtksourceview-2.0` \
			`pkg-config --cflags pygtk-2.0` \
			-I/usr/include/python2.5

LIBS += `pkg-config --libs gtksourceview-2.0` \
		`pkg-config --libs gmodule-2.0` \
		`pkg-config --libs gthread-2.0` \
		`pkg-config --libs pygtk-2.0` \
		-lpython2.5

# project
# 
CXXFLAGS += -I../../include

SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:%.cpp=$(OBJPATH)/%.o)

TARDIR = ../../bin/extends

TARGETS = $(TARDIR)/pyextend.ext \
		  $(TARDIR)/pyextend.py  \
		  $(TARDIR)/puss.py

all : $(TARGETS)

$(TARDIR)/pyextend.ext : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ -ldl $(LIBS)

$(TARDIR)/pyextend.py : src/pyextend.py
	cp -f $< $@

$(TARDIR)/puss.py : src/puss.py
	cp -f $< $@

clean :
	rm -f $(TARGET)
	rm -rf debug
	rm -rf release

$(OBJPATH)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTKINCS) -o $@ -c $<

$(OBJPATH)/%.d : %.cpp
	@echo $(CXX) -MM -o $@ -c $<
	@rm -f $@
	@mkdir -p $(dir $@)
	@echo -n $@ $(dir $@) > $@.tmp
	@$(CXX) $(CXXFLAGS) -MM $< >> $@.tmp
	@mv $@.tmp $@

-include $(SRCS:%.cpp=$(OBJPATH)/%.d)

