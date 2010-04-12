# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

PROJECTS = puss \
	$(wildcard extends/*) \
	$(wildcard plugins/*)

all : $(PROJECTS:%=.project/%)

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
	@make -Cpuss BUILD=$(BUILD) NOT_USE_D_FILE=1 clean
	@for ext in $(EXTS); do make -C$$ext BUILD=$(BUILD) NOT_USE_D_FILE=1 clean; done
	@for plg in $(PLGS); do make -C$$plg BUILD=$(BUILD) NOT_USE_D_FILE=1 clean; done
	@echo make clean finished!

deb :
	@./make_deb

