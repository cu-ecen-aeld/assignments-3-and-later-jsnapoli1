# Makefile for aesdsocket
# Supports cross-compilation via Buildroot TARGET_CONFIGURE_OPTS

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g -DUSE_AESD_CHAR_DEVICE=1
LDFLAGS += -pthread
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
