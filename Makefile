CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
SANITIZE_FLAGS = -fsanitize=address,undefined -g -O1
TARGET = maze
SRC = main.c

.PHONY: all sanitize clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

# 通常ビルドと同じ成果物を上書きするため、必ず clean してから作り直す
sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: clean $(TARGET)

clean:
	rm -f $(TARGET) *.o
