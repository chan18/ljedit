# makefile

TARGET_PATH := ../../../bin/plugins

CC := gcc

.PHONY : debug release clean

release:
	$(CC) -O3 -Wall -DUNICODE -shared -o $(TARGET_PATH)/vconsole.dll *.c

debug:
	$(CC) -g -Wall -D_DEBUG -DUNICODE -shared -o $(TARGET_PATH)/vconsole_d.dll *.c

clean:
	rm -f $(TARGET_PATH)/vconsole.dll
	rm -f $(TARGET_PATH)/vconsole_d.dll

