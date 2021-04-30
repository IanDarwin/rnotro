DEFS=-DCHKINDEX
CFLAGS=-O $(DEFINES)
FILES = README rnotroff.[1c] makefile lint runoffmacros
LINTFLAGS = -ha

runoff:		runoff.c
		$(CC) $(CFLAGS) $(DEFS) runoff.c -o $@

rnotroff:	rnotroff.c
		$(CC) $(CFLAGS) rnotroff.c -o $@

lint:		rnotroff.c
		lint $(LINTFLAGS) $? >$@

dist:		$(FILES)
		@bundle $(FILES)
