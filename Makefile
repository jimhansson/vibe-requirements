.PHONY: all clean test

all:
	$(MAKE) -C src

test:
	$(MAKE) -C src test

clean:
	$(MAKE) -C src clean
