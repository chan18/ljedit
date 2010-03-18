# makefile

TARGET_NAME = gdbmi

all : target vconsole

include ../puss_plugin_makefile_c_template

ifeq ($(PLATFORM), win32)
	CFLAGS += -Ivconsole
	ifeq ($(BUILD), release)
		VCONSOLE = $(TARGET_PATH)/vconsole.dll
	else
		VCONSOLE = $(TARGET_PATH)/vconsole_d.dll
	endif

	vconsole : vconsole/*.c
		ifeq ($(BUILD), release)
			$(CC) -O3 -Wall -DUNICODE -shared -o $@ $^
		else
			$(CC) -g -Wall -D_DEBUG -DUNICODE -shared -o $@ $^
		endif

else
	CFLAGS += -lvte
	vconsole : 

endif
