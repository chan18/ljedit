# makefile

TARGET_NAME = puss_vconsole

ifneq ($(PLATFORM), win32)
	CFLAGS += `pkg-config --cflags vte`
	LIBS += `pkg-config --libs vte`
endif

include ../puss_plugin_makefile_c_template

