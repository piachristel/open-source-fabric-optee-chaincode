## Deployment of chaincode_proxy and chaincode on Raspberry Pi

Technical requirements:
Raspberry Pi 3, Model B

Sources for this installation guide:
based on https://optee.readthedocs.io/building/devices/rpi3.html#build-instructions

**1. Install OP-TEE, cross-compile gRPC and build chaincode_proxy and chaincode**

The following steps need to be executed on any machine which supports the build process.

First execute the steps 1-5 of https://optee.readthedocs.io/building/gits/build.html#get-and-build-the-solution. 
To automatize these steps, you can use the script [install-optee.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/install-optee.sh) with the following parameters:
* change the `optee-project` directory to another directory name, e.g. `optee-rp-project`
* for `get_source_code` function pass `rpi3` as TARGET and `3.5.0` (latest) as branch


Cross-compile gRPC as static library, you can use the script [install-grpc.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/install-grpc.sh) for that step.


Then clone the https://github.com/piachristel/open-source-fabric-optee-chaincode, copy the `chaincode_proxy` and the `chaincode` directory to `{$OPTEE_RAPI-SRC}/optee_examples`.
Copy all header files related to gRPC (may be in `/usr/local/include`dir) to `${OPTEE_RAPI_SRC}/optee_examples/chaincode_proxy/host/include` dir.

Make sure that the include paths for the gRPC static library are correctly set in `${OPTEE_RAPI_SRC}/optee_examples/chaincode_proxy/CMakelists.txt`.

In `${OPTEE_RAPI_SRC}/optee_examples/chaincode_proxy/host` run (source of commands: https://github.com/grpc/grpc): 
```
grpc_cpp_plugin=$(which grpc_cpp_plugin)
protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=$grpc_cpp_plugin invocation.proto
protoc -I . --cpp_out=. invocation.proto
```

Add OpenSSH package (https://git.busybox.net/buildroot/tree/package) to `${OPTEE_RAPI_SRC}/build/common.mk` in the Buildroot section next to the openssl package:
```
@echo "BR2_PACKAGE_OPENSSH=y" >> ../out-br/extra.conf
```

To make the changes effective, you need to run `make clean; make buildroot-clean; make qemu-clean; make -j$(nproc)` in `${OPTEE_RAPI_SRC}/build`.
Once step 7 has been executed, one can connect from machine of step 1 to the Raspberry Pi with:
```
ssh root@ip_of_raspberry_pie
```

**2. SD-card**

Insert the SD-card into your laptop. With `dmesg -T` you can check the name of the SD-card (usually the last output of the command) or in the `/dev` dir you should also see the name added/removed when mounting/unmounting the SD-card.
In my case the name is `mmcblok0` with partitions `mmcblk0p1` and `mmcblk0p2`.

Check the installation guidelines of `make img-help` (in `{$OPTEE_RAPI-SRC}/build/` directory) and execute them on the device where you have inserted the SD-card. Consider the following points:
* Before starting to follow the instructions, check that the SD-card is inserted, but not mounted.
* In the steps `gunzip -cd /home/ubuntu/optee-rapi-project/build../out-br/images/rootfs.cpio.gz | sudo cpio -idmv ("boot/*")` you have first to copy this .cioo.gz files from the machine in step 1 to your local machine.
* In case you are asked if any signature should be removed, remove it.
* Before unmounting the SD-card (last step of `make img-help`), do:
```
// add the ssh key
cd rootfs/
sudo mkdir -m 700 root/.ssh
sudo vim root/.ssh/authorized_keys // add ~/.ssh/id_rsa.pub key
// check that the file properties are correct (otherwise change with chmod):
ls -la rootfs/root/ // should be: drwx------ 2 user users 4096 13. Jun 11:30 .ssh
ls -l rootfs/root/.ssh/ // should be: -rw------- 1 user users XXX 13. Jun 11:31 authorized_keys
```

**3. SD-card at Raspberry Pi**

Put the SD-card into the Raspberry Pi.

**4. UART cable**

Plug in the UART cable at the Raspberry Pi and at the laptop.
For the Raspberry Pi side make sure that the following holds (see also [UART.jpeg](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/UART.jpeg)):
* Black UART cable (ground) matches the RaPi ground pin.
* Yellow UART cable (receive) matches the RaPi UART transmit pin.
* Orange UART cable (transmit) matches the RaPi UART receive pin.

For more information, see:
* https://www.jameco.com/Jameco/workshop/circuitnotes/raspberry-pi-circuit-note.html
* https://www.techexpress.co.nz/products/usb-to-rj45-rs232-serial-cisco-console-cable?variant=37700860688

Check The USB interface of the UART cable on the laptop with `dmesg -T | grep -e USB` (in my case: ttyUSB0).

**5. Mincom**

On your laptop run:
```
sudo minicom -s
// choose 'Serial port setup' with Enter key 
// make configs:
// A /dev/ttyUSB0
// E 115200 8N1
// F No
// G No
// then 'Save setup as dfl'
// then 'Exit' (only leaving config, not leaving minicom)
```

**6. Power Raspberry Pi**

Attach the power cable at the Raspberry Pi (never do that before connected with minicom in step 5)

**7. Ethernet**

Attach the Ethernet cable to the Raspberry Pi.

If the Rasperry Pi has no IP yet, get one with `udhcpc`.

Add this IP to the `chaincode_wrapper`.

**8. Shut down** (only execute this step if running is finished)

* In the UART console do `poweroff`, this shuts down the Rasperry Pi.
* When no more messages are printed in the UART console, unplug the power cable at the Raspberry Pi.
* In the minicom console type Ctrl+A Z X, leave minicom yes.
* Unplug the UART cable at the Raspberry Pi and at the laptop.
