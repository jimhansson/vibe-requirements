CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS  = -lyaml
TARGET  = vibe-req
SRCS    = main.c requirement.c yaml_simple.c discovery.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
