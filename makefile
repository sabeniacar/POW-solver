#user_name: sabeniacar 
#student_id:838392
CC = gcc
CFLAGS = -Wall -std=gnu99 -I.
LDFLAGS = -lm -pthread

SRC = server.c
EXE = server
OBJECT = sha256.c

$(EXE): $(SRC)
	$(CC) $(OBJECT) -o $(EXE) $(SRC) $(LDFLAGS) $(CFLAGS) 

clobber:clean
	/bin/rm $(EXE).
