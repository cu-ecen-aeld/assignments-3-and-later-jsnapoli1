CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
LDFLAGS ?=
TARGET := aesdsocket
SRCS := aesdsocket.c
OBJS := $(SRCS:.c=.o)

.PHONY: all default clean

default: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
