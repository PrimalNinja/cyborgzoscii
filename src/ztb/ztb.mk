# ZOSCII Tamperproof Blockchain - Makefile
# (c) 2025 Cyborg Unicorn Pty Ltd. - MIT License

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = 

# Detect OS
ifeq ($(OS),Windows_NT)
    EXT = .exe
    RM = del /Q
else
    EXT =
    RM = rm -f
endif

# Targets
TARGETS = ztbcreate$(EXT) ztbaddblock$(EXT) ztbaddbranch$(EXT) ztbfetch$(EXT) ztbverify$(EXT)

# Object files
COMMON_OBJ = ztbcommon.o

all: $(TARGETS)

# Compile common functions
ztbcommon.o: ztbcommon.c ztbcommon.h
	$(CC) $(CFLAGS) -c ztbcommon.c -o ztbcommon.o

# Build each tool
ztbcreate$(EXT): ztbcreate.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) ztbcreate.c $(COMMON_OBJ) -o ztbcreate$(EXT) $(LDFLAGS)

ztbaddblock$(EXT): ztbaddblock.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) ztbaddblock.c $(COMMON_OBJ) -o ztbaddblock$(EXT) $(LDFLAGS)

ztbaddbranch$(EXT): ztbaddbranch.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) ztbaddbranch.c $(COMMON_OBJ) -o ztbaddbranch$(EXT) $(LDFLAGS)

ztbfetch$(EXT): ztbfetch.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) ztbfetch.c $(COMMON_OBJ) -o ztbfetch$(EXT) $(LDFLAGS)

ztbverify$(EXT): ztbverify.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) ztbverify.c $(COMMON_OBJ) -o ztbverify$(EXT) $(LDFLAGS)

# Clean build artifacts
clean:
	$(RM) $(TARGETS) *.o

# Install (optional - adjust paths as needed)
install: all
	@echo "Install targets to /usr/local/bin or desired location"

.PHONY: all clean install