  
INCDIR = ./include
OBJDIR = ./bin
BUILDDIR = ./build
SRCDIR = ./source

CC=gcc
CFLAGS=-Wall -g
TARGET = travelMonitor monitor
VALFLAGS = --leak-check=full --track-origins=yes -s --trace-children=yes
# --show-leak-kinds=all
ARGS = -i input_dir -s 100000 -b 100 -m 5

DEPS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

OBJS_TRAVELMON := $(filter-out $(OBJDIR)/monitor.o, $(OBJS))
OBJS_MONITOR := $(filter-out $(OBJDIR)/travelMonitor.o, $(OBJS))

.PHONY: clean run all

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) 
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCDIR)

all: $(TARGET)

travelMonitor: $(filter-out $(OBJDIR)/monitor.o, $(OBJS))
	@mkdir -p $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/$@ $^

monitor: $(filter-out $(OBJDIR)/travelMonitor.o, $(OBJS))
	@mkdir -p $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/$@ $^

run:
	$(BUILDDIR)/travelMonitor ${ARGS}

valgrind:
	valgrind ${VALFLAGS} $(BUILDDIR)/travelMonitor ${ARGS}

clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/*
	rmdir $(OBJDIR) $(BUILDDIR)