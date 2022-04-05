INCDIR = ./include
OBJDIR = ./bin
BUILDDIR = ./build
SRCDIR = ./src

CC=gcc
CFLAGS=-Wall -g
TARGET = vaccineMonitor
VALFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes -s

ARGS = -c inputFile.txt  -b 100000

DEPS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: clean run

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) 
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCDIR)

$(BUILDDIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) -o $@ $^

run:
	$(BUILDDIR)/$(TARGET) ${ARGS}

valgrind:
	valgrind ${VALFLAGS} $(BUILDDIR)/$(TARGET) ${ARGS}

clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/*
	rmdir $(OBJDIR) $(BUILDDIR)