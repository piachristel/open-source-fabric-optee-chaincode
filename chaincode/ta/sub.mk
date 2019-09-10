# source: file bases on https://github.com/linaro-swg/optee_examples/tree/master/hello_world

global-incdirs-y += include
srcs-y += coffee_tracking_chaincode.c
srcs-y += chaincode_library.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
