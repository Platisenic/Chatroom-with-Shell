CC = g++
CFLAGS = -Wall -Iinclude -O2 -std=c++11
OBJDIR = obj
LIBDIR = lib

SRCS   = $(basename $(notdir $(wildcard lib/*.cpp)))
OBJS := $(SRCS:%=obj/%.o)
EXEC   = np_simple

.PHONY: prepare clean all run

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(LIBDIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf test*.txt
	rm -rf $(OBJDIR)/* $(EXEC)

run: all
	./$(EXEC) 3334

