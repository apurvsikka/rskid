# Compiler and flags
CC       := gcc
CXX      := g++
CFLAGS   := -Wall -Wextra -O2
CXXFLAGS := -Wall -Wextra -O2 -std=c++17

# Directories
PREFIX   := /usr/local
BINDIR   := $(PREFIX)/bin

# Targets
TARGET   := rskid
SRC      := main.c
OBJ      := $(SRC:.c=.o)

# Default build
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Install binary and base config
install: $(TARGET)
	@echo ">>> Installing Ruskid to $(BINDIR)"
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo ">>> Done. Ruskid is installed system-wide."

# Uninstall
uninstall:
	@echo ">>> Removing Ruskid..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(SYSCONFDIR)

# Cleanup
clean:
	rm -f $(TARGET) *.o

.PHONY: all install uninstall clean
l:
	@echo ">>> Removing Ruskid..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

# Cleanup
clean:
	rm -f $(TARGET) *.o

.PHONY: all install uninstall clean
