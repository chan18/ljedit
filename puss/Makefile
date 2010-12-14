# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

PROJECTS =	puss \
			$(wildcard extends/*) \
			$(wildcard plugins/*)

PRJS = $(filter-out plugins/puss_plugin_makefile_c_template, $(PROJECTS))

all : $(PRJS:%=.project/%)

.project/% : %
	-make BUILD=$(BUILD) -C$<

strip:
	@for ext in $(wildcard bin/extends/*.ext); do strip $$ext; done
	@for plg in $(wildcard bin/plugins/*.so); do strip $$plg; done
	@for dll in $(wildcard bin/plugins/*.dll); do strip $$dll; done
	@-strip bin/*.so
	@-strip bin/puss
	@-strip bin/*.dll
	@-strip bin/*.exe

clean:
	@for p in $(PROJECTS); do make -C$$p BUILD=$(BUILD) NOT_USE_D_FILE=1 clean; done
	@echo make clean finished!

deb :
	@./make_deb

