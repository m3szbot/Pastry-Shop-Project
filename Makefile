CC = gcc
CFLAGS += -Wall -Werror -std=gnu11 -O2
LDFLAGS += -lm

SRC = source.c
TARGET = source

# Default rule: build the executable
all: $(TARGET)

# Link and compile in one step
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)