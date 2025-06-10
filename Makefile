CC = gcc
CFLAGS = -Wall -Iinclude
TARGET = aiGateway_learn

$(TARGET): main.c src/capture.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c src/capture.c

clean:
	rm -f $(TARGET) *.yuv
	@echo "clear done"

test: $(TARGET)
	./$(TARGET)

.PHONY: clean test