CC	= gcc
FLAGS	= -Wall -g
LIB	= -lm

EXEC 	= partition_record_reader
SRC 	= partition_record_reader.c
BIN 	= bin

all: $(SRC)
	$(CC) $(FLAGS) -o $(BIN)/$(EXEC) $(SRC) $(LIB)

clean:
	rm -rf $(BIN)/*
