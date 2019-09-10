## Deployment of chaincode_proxy and chaincode emulated with QEMU

**1. Install OP-TEE and cross-compile gRPC**

!  Always connect to your host machine with ```ssh: $ ssh host_machine -X```. The *-X* option is used for the correct start of the normal and secure world UARTs.


Execute the steps 1-5 of https://optee.readthedocs.io/building/gits/build.html#get-and-build-the-solution. 
To automatize these steps, you can use the script [install-optee.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/install-optee.sh) with the following parameters:
* for `get_source_code` function pass `qemu_v8` as TARGET and `3.5.0` (latest) as branch


To omit networking issues and to get an ip for the `chaincode_proxy`, we need to create a two-way network bridge between the physical host and the machine emulated by QEMU. First, execute the [qemu_br_tap.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/qemu_br_tap.sh) script as sudo with the ```up```option in the host machine.  Make sure that you substitute *X* in  ```IFDEV="ensX"``` and *ip_address* in ```GATEWAY="ip_address"``` with the correct values. You can look up the first value by running ```$ ip addr show``` on your host machine and the second value refers to the subnet (of IUUN cluster).
   Second, you also need to modify ```${OPTEE_SRC}/build/common.mk``` (again in the host machine) as visible in the box below. With these changes, you disalbe the GDB server and the SLIRP networking. Furthermore, you declare the name of the TAP device (identical to the one in the ```qemu_br_tap.sh```) and define a random MAC address for this TAP device. The QEMU_EXTRA_ARGS in the end ensure the setup of the bridge networking.
```
#ifeq ($(GDBSERVER),y)
#HOSTFWD := ,hostfwd=tcp::12345-:12345
#endif
# Enable QEMU SLiRP user networking
#QEMU_EXTRA_ARGS +=\
#        -netdev user,id=vmnic$(HOSTFWD) -device virtio-net-device,netdev=vmnic
GUEST_TAP_NAME ?= tap2
GUEST_MAC_ADDR ?= 02:16:1e:02:01:02
QEMU_EXTRA_ARGS += \
        -device virtio-net-device,mac=$(GUEST_MAC_ADDR),netdev=vmnic \
        -netdev tap,id=vmnic,ifname=$(GUEST_TAP_NAME),script=no,downscript=no
```
  For making the changes effective, run  ```make clean; make linux-clean; make buildroot-clean; make qemu-clean; make -j$(nproc)``` in ```${OPTEE_SRC}/build``` dir.


Cross-compile gRPC as static library, you can use the script [install-grpc.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/install-grpc.sh) for that step.

**2. Build chaincode_proxy**

Then checkout this repository and make a symlink from the `chaincode_proxy` directory to `${OPTEE_SRC}/optee_examples/` (use absolute paths!, make sure that the `chaincode_proxy` dir in `optee_examples` does not exist yet): 
```
ln -s ${REPO}/chaincode_proxy/  ${OPTEEP_SRC}/optee_examples/chaincode_proxy
```
Copy all header files related to gRPC (may be in `/usr/local/include`dir) to `${OPTEE_SRC}/optee_examples/chaincode_proxy/host/include` dir.

Make sure that the include paths for the gRPC static library are correctly set in `${OPTEE_SRC}/optee_examples/chaincode_proxy/CMakelists.txt`.

In `${OPTEEP_SRC}/optee_examples/chaincode_proxy/host` run (source of commands: https://github.com/grpc/grpc): 
```
grpc_cpp_plugin=$(which grpc_cpp_plugin)
protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=$grpc_cpp_plugin invocation.proto
protoc -I . --cpp_out=. invocation.proto
```

To make the changes effective, you need to run `make clean; make buildroot-clean; make qemu-clean; make -j$(nproc)` in `${OPTEE_SRC}/build`.

**3. Build chaincode**

Checkout this repository and make a symlink from the `chaincode` directory to `${OPTEE_SRC}/optee_examples/` (use absolute paths!, make sure that the `chaincode` dir in `optee_examples` does not exist yet): 
```
ln -s ${REPO}/chaincode/  ${OPTEEP_SRC}/optee_examples/chaincode
```

To make the changes effective, you need to run `make clean; make buildroot-clean; make qemu-clean; make -j$(nproc)` in `${OPTEE_SRC}/build`.
