CC       = gcc
SOURCES  = sample.c
OBJS     = $(SOURCES:.c=.o)
INCLUDES = -I$(HOME)/opt/include  \
           -I$(HOME)/opt/include/apr-1
CFLAGS   = -Wall
LDFLAGS  = -L$(HOME)/opt/lib -lapr-1
LIBS     = 
TARGET   = main

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

debug:
	make -f Makefile CFLAGS="-g $(CFLAGS)" LDFLAGS="-g $(LDFLAGS)"

clean:
	rm -f $(TARGET) $(OBJS)

run:
	@LD_LIBRARY_PATH=/home/foo/opt/lib  ./$(TARGET)

