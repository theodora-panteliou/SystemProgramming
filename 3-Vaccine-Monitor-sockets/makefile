  
INCDIR = ./include
OBJDIR = ./bin
BUILDDIR = ./build
SRCDIR = ./source

CC=gcc
CFLAGS=-Wall -g
TARGET = travelMonitorClient monitorServer
VALFLAGS = --leak-check=full --track-origins=yes --trace-children=yes
# --show-leak-kinds=all
ARGS = -m 5 -b 100 -c 30 -s 100000 -i input_dir4 -t 1

DEPS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: clean run all

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) 
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCDIR)

all: $(TARGET)

travelMonitorClient: $(filter-out $(OBJDIR)/monitorServer.o, $(OBJS))
	@mkdir -p $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/$@ $^

monitorServer: $(filter-out $(OBJDIR)/travelMonitorClient.o, $(OBJS))
	@mkdir -p $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/$@ $^ -lpthread

run:
	$(BUILDDIR)/travelMonitorClient ${ARGS}

valgrind:
	valgrind ${VALFLAGS} $(BUILDDIR)/travelMonitorClient ${ARGS}

clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/*
	rmdir $(OBJDIR) $(BUILDDIR)