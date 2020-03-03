DIR_BIN = .
DIR_OBJ = ./obj
DIR_SRC = .

SRC = $(wildcard *.cc )
OBJ = $(patsubst %.cc,$(DIR_OBJ)/%.o,$(notdir $(SRC)))

CXX_FLAG = -g -Wall -std=c++11 -pthread -O3
CC = g++

TARGET = test12

$(DIR_BIN)/$(TARGET) : $(OBJ)
	$(CC) $(CXX_FLAG) -o $@ $^

$(DIR_OBJ)/%.o : $(DIR_SRC)/%.cc
	if [ ! -d $(DIR_OBJ) ];	then mkdir -p $(DIR_OBJ); fi;
	$(CC) $(CXX_FLAG) -c $< -o $@

.PHONY : clean
clean : 
	-rm -rf $(DIR_OBJ)