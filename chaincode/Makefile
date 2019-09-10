# source: file bases on https://github.com/linaro-swg/optee_examples/tree/master/hello_world
export V?=0

# If _TA specific compilers are not specified, then use CROSS_COMPILE
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)

.PHONY: all
all:
	$(MAKE) -C ta CROSS_COMPILE="$(TA_CROSS_COMPILE)" LDFLAGS=""

.PHONY: clean
clean:
	$(MAKE) -C ta clean
