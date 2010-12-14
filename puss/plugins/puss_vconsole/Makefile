# makefile

TARGET_NAME = puss_vconsole

all : target vconsole

include ../puss_plugin_makefile_c_template

ifeq ($(PLATFORM), win32)
	CFLAGS += -Ivconsole
	ifeq ($(BUILD), release)
		VCONSOLE = $(TARGET_PATH)/vconsole.dll
	else
		VCONSOLE = $(TARGET_PATH)/vconsole_d.dll
	endif

vconsole : $(VCONSOLE)

$(VCONSOLE) : vconsole/*.c
ifeq ($(BUILD), release)
	$(CC) -O3 -Wall -DUNICODE -shared -o $@ $^
else
	$(CC) -g -Wall -D_DEBUG -DUNICODE -shared -o $@ $^
endif

else
	CFLAGS += `pkg-config --cflags vte`
	LIBS += `pkg-config --libs vte`

vconsole : 
	echo 'sss'

endif
