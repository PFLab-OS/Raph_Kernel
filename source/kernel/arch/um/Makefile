OBJ = main.o

BIN = kernel

INCLUDE_PATH = -I$(realpath ../../)
RAPHFLAGS = -ggdb3 -O0 -Wall -Werror=return-type -Wshadow -D__KERNEL__ -D_KERNEL -DARCH=$(ARCH) -Dbootverbose=0 $(INCLUDE_PATH) -MMD -MP
RAPHCFLAGS = $(RAPHFLAGS) -std=c99
RAPHCXXFLAGS = $(RAPHFLAGS) -std=c++1y
CFLAGS += $(RAPHCFLAGS)
CXXFLAGS += $(RAPHCXXFLAGS)
ASFLAGS += $(RAPHFLAGS)
export CFLAGS
export CXXFLAGS
export ASFLAGS

default:

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ -o $@

run: $(BIN)
	./$(BIN)

clean:
	-rm -f $(BIN)
