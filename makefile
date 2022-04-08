# +mkmake+ -- Everything after this line is automatically generated

EXECUTABLES = forkcat1 forkcat2 headsort

all: $(EXECUTABLES)

clean:
	rm -f core *.o $(EXECUTABLES) a.out

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c


forkcat1: forkcat1.c
	$(CC) $(CFLAGS) -o forkcat1 forkcat1.c

forkcat2: forkcat2.c
	$(CC) $(CFLAGS) -o forkcat2 forkcat2.c

headsort: headsort.c
	$(CC) $(CFLAGS) -o headsort headsort.c
