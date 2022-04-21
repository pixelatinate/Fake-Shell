CC = gcc 
INCLUDES = -I/home/jplank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/jplank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a 
EXECUTABLES = jsh

all: $(EXECUTABLES)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

jsh: jsh.o
	$(CC) $(CFLAGS) -o jsh jsh.o $(LIBS)

clean:
	rm -rf core $(EXECUTABLES) *.o tmp*.txt f*.txt