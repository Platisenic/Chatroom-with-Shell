CC = g++
CFLAGS = -Wall -Iinclude -g -std=c++11
OBJDIR = obj
LIBDIR = lib

EXEC1   = np_simple
EXEC1HEADER   = $(wildcard include/$(EXEC1)/*.hpp)

EXEC2   = np_single_proc
EXEC2HEADER   = $(wildcard include/$(EXEC2)/*.hpp)

EXEC3   = np_multi_proc
EXEC3HEADER   = $(wildcard include/$(EXEC3)/*.hpp)

.PHONY: prepare clean all run1 run2 run3

all: $(EXEC1) $(EXEC2)
# all: $(EXEC1) $(EXEC2) $(EXEC3)

$(EXEC1): $(EXEC1).cpp $(EXEC1HEADER)
	$(CC) $(CFLAGS) -o $@ $<

$(EXEC2): $(EXEC2).cpp $(EXEC2HEADER)
	$(CC) $(CFLAGS) -o $@ $<

# $(EXEC3): $(EXEC3).cpp $(EXEC3HEADER)
# 	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf test*.txt
	rm -rf $(EXEC1) $(EXEC2) $(EXEC3)

run1: $(EXEC1)
	./$(EXEC1) 3334

run2: $(EXEC2)
	./$(EXEC2) 3335

# run3: $(EXEC3)
# 	./$(EXEC3) 3336

prepare:
	mkdir -p bin
	cp `which ls` bin/ls
	cp `which cat` bin/cat
	g++ commands/noop.cpp -o bin/noop
	g++ commands/number.cpp -o bin/number
	g++ commands/removetag0.cpp -o bin/removetag0
	g++ commands/removetag.cpp -o bin/removetag
