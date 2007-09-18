# makefile ljcs
# 

CXX = g++
#CXXFLAGS = -g -DSHOW_PARSE_DEBUG_INFOS
CXXFLAGS = -o2 -DNDEBUG

CPOBJS = src/cps/next_block.o   \
	src/cps/utils.o         \
	src/cps/cp_block.o      \
	src/cps/cp_using.o      \
	src/cps/cp_namespace.o  \
	src/cps/cp_var.o        \
	src/cps/cp_function.o   \
	src/cps/cp_class.o      \
	src/cps/cp_enum.o       \
	src/cps/cp_typedef.o    \
	src/cps/cp_template.o

PYTHON_INC=/usr/include/python2.5
PYTHON_LIB=python2.5

#PYTHON_INC=/usr/include/python2.4
#PYTHON_LIB=python2.4

#LJMLOBJS = src/mlexer_d.o src/parser.o src/main_ml.o src/ds_utils.o
LJCSOBJS = src/use_mlexer.o src/parser.o src/cp.o src/skey.o src/searcher.o src/ds_utils.o $(CPOBJS)
LJCS_PYOBJS = $(LJCSOBJS) src/ljcs_py_wrap.o

all : lib/libljcs.so	# lib/_ljcs_py.so

lib/libljcs.so : $(LJCSOBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(LJCSOBJS)
	cp src/ds.h include/ljcs/
	cp src/parser.h include/ljcs/
	cp src/searcher.h include/ljcs/
	cp src/skey.h include/ljcs/
	cp src/ljcs.h include/ljcs/
	cp src/crt_debug.h include/ljcs/

src/use_mlexer.o : src/use_mlexer.cpp src/mlexer.cpp src/ml.h src/parser.h src/ds.h
	$(CXX) $(CXXFLAGS) -DYY_STACK_USED -c -o $@ $<

src/mlexer.cpp : src/ml.l
	flex -o src/mlexer.cpp src/ml.l

lib/_ljcs_py.so : $(LJCS_PYOBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(LJCS_PYOBJS) -l$(PYTHON_LIB)
	cp lib/_ljcs_py.so plugin/gedit/ljcs/
	cp lib/ljcs_py.py plugin/gedit/ljcs/

src/ljcs_py_wrap.o : src/ljcs_py_wrap.cpp
	$(CXX) $(CXXFLAGS) -I$(PYTHON_INC) -Iinclude -c -o $@ $<

src/ljcs_py_wrap.cpp : src/ljcs_py.i src/searcher.h
	swig -c++ -python -outdir lib -o ljcs_py_wrap.cpp $<
	mv -f ljcs_py_wrap.* src/

clean :
	rm -f src/mlexer.cpp
	rm -f src/ljcs_py_wrap.*
	rm -f src/*.o
	rm -f src/cps/*.o
	rm -f lib/*
	rm -f include/ljcs/*

