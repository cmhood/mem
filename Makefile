.POSIX:

PREFIX=/usr/local
BINDIR=${PREFIX}/bin

mem: mem.c
	${CC} $< -o $@ -D_POSIX_C_SOURCE ${CPPFLAGS} ${CFLAGS} ${LDFLAGS}

install: mem
	mkdir -p ${BINDIR}
	cp mem ${BINDIR}

uninstall:
	rm -f ${BINDIR}/mem

clean:
	rm -f mem ${OBJ}

.PHONY: clean install uninstall
