dayzserverinfo: main.c
	@gcc -o dayzserverinfo main.c

clean:
	@/bin/rm -f dayzserverinfo

install:
	@/usr/bin/install -v --owner root --group root --mode 0755 dayzserverinfo /usr/local/bin/
