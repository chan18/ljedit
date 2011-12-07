# makefile

TARGET_NAME = puss_vconsole

ifneq ($(OS), Windows_NT)
	ifeq ($(GTK_VERSION),3)
		CFLAGS += `pkg-config --cflags vte-2.90`
		LIBS += `pkg-config --libs vte-2.90`
	else
		CFLAGS += `pkg-config --cflags vte`
		LIBS += `pkg-config --libs vte`
	endif
endif

include ../puss_plugin_makefile_c_template

