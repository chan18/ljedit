# makefile puss
# 
# usage :
#     build debug version   : make
#     build release version : make BUILD=release
# 

EXTS = $(wildcard extends/*)
PLGS = $(wildcard extends/*)

all : .puss .extends .plugins

.puss :
	@make -Cpuss BUILD=$(BUILD);
	@make -Cwin32_boot BUILD=$(BUILD);

.extends :
	@for ext in $(EXTS); do make -C$$ext BUILD=$(BUILD); done

.plugins :
	@for plg in $(PLGS); do make -C$$plg BUILD=$(BUILD); done

clean:
	@make -Cpuss BUILD=$(BUILD) clean
	@for ext in $(EXTS); do make -C$$ext BUILD=$(BUILD) clean; done
	@for ext in $(PLGS); do make -C$$ext BUILD=$(BUILD) clean; done

deb :
	@./make_deb

zip :
	@rm -rf output
	@svn export ./bin output
	@if [ -f bin/_puss.exe ]; then cp -f bin/_puss.exe output/; fi
	@if [ -f bin/puss.exe ]; then cp -f bin/puss.exe output/; fi
	@if [ -f bin/puss ]; then cp -f bin/puss output/; fi
	@cp -rf bin/extends/*.ext output/extends/
	@cp -rf bin/plugins/plugin_* output/plugins/
	@echo tar jcf puss-1.0-`date +%F`.tar.bz2
	@cd output; tar jcf ../puss-1.0-`date +%F`.tar.bz2 *
	@rm -rf output

