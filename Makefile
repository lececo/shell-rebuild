CFLAGS=-g -Wall -pedantic
OBJECTS=shell.o interpretiere.o scanner.o parser.o utils.o kommandos.o listen.o wortspeicher.o
SOURCES=shell.c interpretiere.c scanner.l parser.y utils.c kommandos.c  wortspeicher.c listen.c 
CSOURCES=shell.c interpretiere.c scanner.c parser.c utils.c kommandos.c  wortspeicher.c listen.c 

all: depend shell

scanner.c: scanner.l
	flex -I -oscanner.c scanner.l

parser.c parser.h: parser.y
	bison -o parser.c -dtv $<

scanner.o: scanner.c parser.h utils.h kommandos.h

shell:  $(OBJECTS)
	gcc $(CFLAGS) -o $@  $^

depend: $(CSOURCES)
	 gcc -MM $(CSOURCES) 2> make_depend.log > .depend 
	 grep -v "linker input" make_depend.log | cat > /dev/fd/2
	 rm -f make_depend.log

%.o: %.c 
	       gcc -c $(CFLAGS) $<

clean:
	rm -f $(OBJECTS) parser.c parser.h scanner.c *~ .depend parser.output

dist-clean:
	make clean
	rm -f shell

-include .depend
