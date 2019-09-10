# source: file bases on https://github.com/linaro-swg/optee_examples/tree/master/hello_world
export V?=0

# If _HOST specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)

.PHONY: all
all:
	$(MAKE) -C host CROSS_COMPILE="$(HOST_CROSS_COMPILE)" --no-builtin-variables

.PHONY: clean
clean:
	$(MAKE) -C host clean

