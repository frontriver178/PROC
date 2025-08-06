CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
TARGET = mysh
OBJS = report2_main.o shell.o builtins.o execute.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

report2_main.o: report2_main.c shell.h
	$(CC) $(CFLAGS) -c report2_main.c

shell.o: shell.c shell.h
	$(CC) $(CFLAGS) -c shell.c

builtins.o: builtins.c shell.h
	$(CC) $(CFLAGS) -c builtins.c

execute.o: execute.c shell.h
	$(CC) $(CFLAGS) -c execute.c

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run