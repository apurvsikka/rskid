CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -O2
TARGET=rskid
SOURCE=rskid.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: install clean
