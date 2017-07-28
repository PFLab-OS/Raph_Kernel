OBJ = main.o

BIN = kernel

default:

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ -o $@

run: $(BIN)
	./$(BIN)

clean:
	-rm -f $(BIN)
