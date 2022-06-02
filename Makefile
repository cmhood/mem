.POSIX:

PREFIX=/usr/local
BINDIR=${PREFIX}/bin
MANDIR=${PREFIX}/share/man

mem: mem.c
	${CC} $< -o $@ -D_POSIX_C_SOURCE ${CPPFLAGS} ${CFLAGS} ${LDFLAGS}

install: mem
	mkdir -p ${BINDIR} ${MANDIR}/man1
	cp mem ${BINDIR}
	cp mem.1 ${MANDIR}/man1

uninstall:
	rm -f ${BINDIR}/mem
	rm -f ${MANDIR}/man1/mem.1

clean:
	rm -f mem ${OBJ}

.PHONY: clean install uninstall
