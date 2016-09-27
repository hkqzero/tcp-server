INC = -I./include -I./src/tool/ -I./src/tool/log/
LNKDIR = -L./lib
LNK = 
SRC = ./src/*.cpp ./src/tool/*.cpp ./src/tool/log/*.cpp

CC    = g++
CFLAG = -g -O3 -ggdb -Wall -Wno-deprecated  

EXE  = bin/test

all:
	$(CC) $(CFLAG) $(INC) $(LNKDIR) $(SRC) $(LNK) -o $(EXE)
clean: 
	rm -rf *.o
	rm -rf $(EXE)
