CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
SANITIZE_FLAGS = -fsanitize=address,undefined -g -O1
TARGET = maze
SANITIZE_TARGET = maze-sanitize
SRC = main.c

.PHONY: all sanitize clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

sanitize: $(SANITIZE_TARGET)

$(SANITIZE_TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET) $(SANITIZE_TARGET) *.o
