# makefile ljcs
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

CXXFLAGS = -g #-DSHOW_PARSE_DEBUG_INFOS
OBJPATH = debug

ifeq ($(BUILD), release)
	CXXFLAGS = -O2
	OBJPATH = release
endif

SRCPATH = src

SRCS = $(wildcard $(SRCPATH)/*.cpp $(SRCPATH)/cps/*.cpp)

OBJS = $(SRCS:$(SRCPATH)/%.cpp=$(OBJPATH)/%.o)

all : lib/libljcs.so

LJCS_INCS = ds.h parser.h searcher.h index.h skey.h ljcs.h crt_debug.h

lib/libljcs.so : $(OBJS) $(OBJPATH)/mlexer.o
	$(CXX) $(CXXFLAGS) -shared -o $@ $^
	cp $(LJCS_INCS:%.h=$(SRCPATH)/%.h) include/ljcs/

$(OBJPATH)/%.o : $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJPATH)/mlexer.o : $(SRCPATH)/mlexer.cpp $(SRCPATH)/ml.l $(SRCPATH)/ml.h $(SRCPATH)/parser.h $(SRCPATH)/ds.h
	$(CXX) $(CXXFLAGS) -DYY_STACK_USED -c -o $@ $<

$(SRCPATH)/mlexer.cpp : $(SRCPATH)/ml.l
	flex -o $@ $<

clean :
	rm -rf debug
	rm -rf release
	rm -f lib/*
	rm -f src/mlexer.cpp
	rm -f src/ljcs_py_wrap.*
	rm -f include/ljcs/*

#PYTHON_INC=/usr/include/python2.5
#PYTHON_LIB=python2.5
#LJCS_PYOBJS = $(LJCSOBJS) src/ljcs_py_wrap.o

#lib/_ljcs_py.so : $(LJCS_PYOBJS)
#	$(CXX) $(CXXFLAGS) -shared -o $@ $(LJCS_PYOBJS) -l$(PYTHON_LIB)
#	cp lib/_ljcs_py.so plugin/gedit/ljcs/
#	cp lib/ljcs_py.py plugin/gedit/ljcs/

#src/ljcs_py_wrap.o : src/ljcs_py_wrap.cpp
#	$(CXX) $(CXXFLAGS) -I$(PYTHON_INC) -Iinclude -c -o $@ $<

#src/ljcs_py_wrap.cpp : src/ljcs_py.i src/searcher.h
#	swig -c++ -python -outdir lib -o ljcs_py_wrap.cpp $<
#	mv -f ljcs_py_wrap.* src/
