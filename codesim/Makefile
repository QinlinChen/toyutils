BIN = codesim
OBJ = CharReader.o CodeSim.o Fingerprint.o RollingHash.o Test.o main.o
CC = g++
CXXFLAGS = -std=c++11 -g -O2 -Wall -Werror

$(BIN): $(OBJ)
	$(CC) $(CXXFLAGS) -o $(BIN) $(OBJ)

.PHONY: clean cleanobj

cleanobj:
	rm $(OBJ)

clean: cleanobj
	rm $(BIN)
