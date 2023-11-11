TARGET = fusefs
CC = gcc
CFLAGS = -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS = -lfuse

all: $(TARGET)

$(TARGET): fuse.c
	$(CC) $(CFLAGS) fuse.c -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	@echo "Please enter the mount point:"
	@read dir; ./$(TARGET) $$dir

clean:
	@rm -f $(TARGET)
