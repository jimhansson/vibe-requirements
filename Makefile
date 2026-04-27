.PHONY: all clean test check-compiler

all:
	$(MAKE) -C src

test:
	$(MAKE) -C src test

check-compiler:
	$(MAKE) -C src check-compiler

clean:
	$(MAKE) -C src clean
