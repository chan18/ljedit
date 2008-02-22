# makefile ljcs
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

CXXFLAGS = -g -Wall
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2 -Wall
	OBJPATH = release
endif


SRCS = src/mlexer.cpp $(wildcard src/*.cpp src/cps/*.cpp)

OBJS = $(SRCS:%.cpp=$(OBJPATH)/%.o)

INCPATH  = include/ljcs

INCS = ds.h parser.h searcher.h index.h skey.h ljcs.h crt_debug.h

all : lib/libljcs.a lib/libljcs.so $(INCS:%.h=$(INCPATH)/%.h)

lib/libljcs.a :  $(OBJS)
	$(AR) -rc $@ $^

lib/libljcs.so : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^

$(INCPATH)/%.h : src/%.h
	cp -f $^ $@

clean :
	rm -rf debug
	rm -rf release
	rm -f lib/*
	rm -f src/mlexer.cpp
	rm -f src/ljcs_py_wrap.*
	rm -f $(INCPATH)/*

$(OBJPATH)/src/mlexer.o : src/mlexer.cpp src/ml.l
	$(CXX) $(CXXFLAGS) -DYY_STACK_USED -c -o $@ $<

src/mlexer.cpp : src/ml.l
	flex -o $@ $<

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

