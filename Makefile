CC=gcc
CFLAGS=-c -Wall -Os `pkg-config --cflags x11`
LDFLAGS=`pkg-config --libs x11`
SOURCES=winplacement.c
OBJECTS=$(SOURCES:.c=.o)
	EXECUTABLE=winplacement

all: $(SOURCES) $(EXECUTABLE)
		
$(EXECUTABLE): $(OBJECTS) 
		$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
		$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)
