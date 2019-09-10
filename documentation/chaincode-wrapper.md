## Deployment of chaincode_wrapper

First install fabric, you can use the script [install-fabric.sh](https://github.com/piachristel/open-source-fabric-optee-chaincode/blob/master/documentation/install-fabric.sh) and then run:
```
sudo apt install protobuf-compiler // probably not necessary
```
Then checkout this repository and copy the `chaincode_wrapper` directory to `${GO_SRC}/github.com/hyperledger/fabric/examples/chaincode/go/`(do not use symlink here since it may cause issues together with the docker containers used for instantiating the chaincodes).
Before continuing, clear any data stored by the peer (https://github.com/hyperledger/fabric/blob/release-1.4/sampleconfig/core.yaml) and any existing images or container which are related to the coffee tracking chaincode (https://hyperledger-fabric.readthedocs.io/en/release-1.4/write_first_app.html?highlight=docker%20rm, https://docs.docker.com/engine/reference/commandline/docker/):
```
rm -rf /var/hyperledger/* // to remove data stored by peer
docker rmi ... // to remove images
docker rm ... // to remove container
```
In a first terminal, in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric/sampleconfig` start the orderer (https://hyperledger-fabric.readthedocs.io/en/release-1.4/peer-chaincode-devmode.html):
```
ORDERER_GENERAL_GENESISPROFILE=SampleDevModeSolo ../.build/bin/orderer
```
In a second terminal, in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric/sampleconfig` start the peer (https://hyperledger-fabric.readthedocs.io/en/release-1.4/peer-chaincode-devmode.html):
```
CORE_PEER_CHAINCODELISTENADDRESS=0.0.0.0:7052 ../.build/bin/peer node start
```
In a third terminal, in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric/sampleconfig` create the channel and let the peer join created channel (https://hyperledger-fabric.readthedocs.io/en/release-1.4/peer-chaincode-devmode.html):
```
../.build/bin/configtxgen -channelID ch -outputCreateChannelTx ch.tx -profile SampleSingleMSPChannel
../.build/bin/peer channel create -o 127.0.0.1:7050 -c ch -f ch.tx
../.build/bin/peer channel join -b ch.block
```
In a fourth terminal, in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric` install the version of protoc-gen-go which is venored with fabric, because we need to ensure that the protoc-gen-go version matches the proto package version - version 2 with fabric 1.4 (https://github.com/golang/protobuf, https://github.com/hyperledger/fabric/blob/release-1.4/gotools.mk):
```
make gotool.protoc-gen-go
```
Still in the fourth terminal, but now in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric/examples/chaincode/go/chaincode_wrapper`, build the invocation proto file (https://github.com/grpc/grpc-go):
```
protoc -I proto/ proto/invocation.proto --go_out=plugins=grpc:proto/ 
``` 
Still in the fourth terminal, but now in the directory `${GOPATH}/go/src/github.com/hyperledger/fabric/examples/go/chaincode_wrapper/cmd` build the chaincode (https://hyperledger-fabric.readthedocs.io/en/release-1.4/peer-chaincode-devmode.html):
```
go build -o chaincode_wrapper
``` 
In the third terminal, install, instantiate and invoke the chaincode (https://hyperledger-fabric.readthedocs.io/en/release-1.4/peer-chaincode-devmode.html):
```
$ ../.build/bin/peer chaincode install -n coffee_tracking_chaincode_wrapper -v 0 -p github.com/hyperledger/fabric/examples/chaincode/go/chaincode_wrapper/cmd
$ ../.build/bin/peer chaincode instantiate -n coffee_tracking_chaincode_wrapper -v 0 -c '{"Args":["init"]}' -o 127.0.0.1:7050 -C ch
$ ../.build/bin/peer chaincode invoke -n coffee_tracking_chaincode_wrapper -c '{"Args":["setup","6ceb95bae9b1482aa1bda1ebca0671b9"]}' -o 127.0.0.1:7050 -C ch
$ ../.build/bin/peer chaincode invoke -n coffee_tracking_chaincode_wrapper -c '{"Args":["create","peter", "0"]}' -o 127.0.0.1:7050 -C ch
```
