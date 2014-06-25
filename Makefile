arma2serverinfo: main.c
	@gcc -o arma2serverinfo main.c

clean:
	@/bin/rm -f arma2serverinfo

install: arma2serverinfo
	@/usr/bin/install -v --owner root --group root --mode 0755 arma2serverinfo /usr/local/bin/
