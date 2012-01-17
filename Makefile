CC=`which gcc`
CFLAGS=-c -Wall -Os `pkg-config --cflags x11`
LDFLAGS=`pkg-config --libs x11 xinerama`
SOURCES=winplacement.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=winplacement

all: $(SOURCES) $(EXECUTABLE)
		
$(EXECUTABLE): $(OBJECTS) 
		$(CC) $(CFFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o:%.c
		$(CC) $(CFLAGS) -c $< 

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
