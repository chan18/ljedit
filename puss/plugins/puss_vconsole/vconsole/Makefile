# makefile

TARGET_PATH = ../../../bin/plugins

CFLAGS += -Ivconsole
ifeq ($(BUILD), release)
	VCONSOLE = $(TARGET_PATH)/vconsole.dll
else
	VCONSOLE = $(TARGET_PATH)/vconsole_d.dll
endif

release : $(VCONSOLE)

debug: $(VCONSOLE)

$(VCONSOLE) : vconsole/*.c
ifeq ($(BUILD), release)
	$(CC) -O3 -Wall -DUNICODE -shared -o $@ $^
else
	$(CC) -g -Wall -D_DEBUG -DUNICODE -shared -o $@ $^
endif
