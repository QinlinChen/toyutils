BIN = ping
OBJ = ping.o error.o
CC = gcc
CFLAGS = -g -O2 -Wall

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

test: $(BIN)
	./$(BIN) www.baidu.com

.PHONY: clean cleanobj

clean: cleanobj
	-rm $(BIN)

cleanobj:
	-rm $(OBJ)
