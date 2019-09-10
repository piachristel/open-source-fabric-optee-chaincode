// examples that helped me to write my code (no copy paste)
// https://github.com/hyperledger/fabric/blob/release-1.4/examples/chaincode/go/example02/chaincode.go
// https://github.com/pahanini/go-grpc-bidirectional-streaming-example
// https://github.com/grpc/grpc-go/blob/master/examples/route_guide/client/client.go
// https://github.com/grpc/grpc-go/blob/master/interop/test_utils.go
// https://gist.github.com/jzelinskie/10ceca82f4f5085c106d
// https://medium.com/namely-labs/go-protocol-buffers-57b49e28bc4a
// https://github.com/golang/protobuf/issues/205
// https://grpc.io/blog/deadlines/

// used naming conventions:
// https://golang.org/doc/effective_go.html

package chaincode_wrapper

import (
	"context"
	"encoding/hex"
	"fmt"
	"time"

	"github.com/hyperledger/fabric/core/chaincode/shim"
	grpcpb "github.com/hyperledger/fabric/examples/chaincode/go/chaincode_wrapper/proto"
	pb "github.com/hyperledger/fabric/protos/peer"
	"google.golang.org/grpc"
)

// stores uuid used by chaincode_proxy to invoke chaincode inside the secure world
type ChaincodeWrapper struct {
	Uuid []byte // for testing, uuid needs to be accessible outside
}

const (
	address = "172.28.10.104:50051"
)

// instantiate chaincode_wrapper
func (t *ChaincodeWrapper) Init(stub shim.ChaincodeStubInterface) pb.Response {
	return shim.Success(nil)
}

// invoke chaincode_wrapper
func (t *ChaincodeWrapper) Invoke(stub shim.ChaincodeStubInterface) pb.Response {
	function, _ := stub.GetFunctionAndParameters()
	if function == "setup" {
		return t.setup(stub)
	} else {
		return t.invoke(stub)
	}
}

// store uuid in ChaincodeWrapper
func (t *ChaincodeWrapper) setup(stub shim.ChaincodeStubInterface) pb.Response {
	// check number of args
	_, args := stub.GetFunctionAndParameters()
	if len(args) != 1 {
		return shim.Error("Failed to invoke setup function of chaincode_wrapper with error: incorrect number of args, expecting 1 arg (uuid).")
	}

	// read args
	uuidInput := args[0]

	// store uuid
	var err error
	t.Uuid, err = hex.DecodeString(uuidInput)
	if err != nil {
		return shim.Error(fmt.Sprintf("Failed to invoke setup function of chaincode_wrapper with error: %s", err))
	}
	return shim.Success(nil)
}

// forward invocation via gRPC to chaincode_wrapper, handle GetStateRequest and PutStateRequest and finally receive InvocationResponse
func (t *ChaincodeWrapper) invoke(stub shim.ChaincodeStubInterface) pb.Response {
	// check number of args
	function, args := stub.GetFunctionAndParameters()
	if len(args) < 1 {
		return shim.Error("Failed to invoke chaincode_wrapper with error: incorrect number of args, expecting at least name of function to execute")
	}

	// set up the grpc client connection towards the chaincode_proxy
	conn, err := grpc.Dial(address, grpc.WithInsecure())
	if err != nil {
		return shim.Error(err.Error())
	}
	defer conn.Close() // https://godoc.org/google.golang.org/grpc#ClientConn.NewStream
	client := grpcpb.NewInvocationClient(conn)

	// Setup context with timeout.
	// chaincode_wrapper waits at max 10 minutes for the gRPC to complete.
	// After the timeout the start of the gRPC call TransactionInvocation and
	// any send or receive will fail with an error. This error is caught by a cleanup.
	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(10)*time.Minute) // client wait at max 10 minutes for the gRPC to complete                                                                    // cancel context (https://godoc.org/google.golang.org/grpc#ClientConn.NewStream)
	defer cancel()                                                                          // https://godoc.org/google.golang.org/grpc#ClientConn.NewStream

	// create the grpc client stream
	stream, err := client.TransactionInvocation(ctx) // if no timeout do client.TransactionInvocation(context.Background()) (not used)
	// ctx := stream.Context // returns context (not used)
	if err != nil {
		return shim.Error(err.Error())
	}

	// prepare and send transaction invocation to chaincode_proxy
	wrapperMsg := &grpcpb.ChaincodeWrapperMessage{
		MessageOneof: &grpcpb.ChaincodeWrapperMessage_InvocationRequest{
			InvocationRequest: &grpcpb.InvocationRequest{
				FunctionName:  function,
				ChaincodeUuid: t.Uuid,
				Arguments:     args,
			},
		},
	}
	err = stream.Send(wrapperMsg)
	if err != nil {
		return shim.Error(err.Error())
	}

	for {
		// wait for the message of the chaincode_proxy and act accordingly
		proxyMsg, err := stream.Recv()
		if err != nil {
			return shim.Error(err.Error())
		}

		switch u := proxyMsg.Type.(type) {
		case *grpcpb.ChaincodeProxyMessage_InvocationResponse:
			{
				// read execution response of InvocationResponse gRPC message from chaincode_proxy
				executionResponse := u.InvocationResponse.ExecutionResponse
				return shim.Success([]byte(executionResponse))
			}
		case *grpcpb.ChaincodeProxyMessage_GetStateRequest:
			{
				// read key of GetStateRequest gRPC message from chaincode_proxy
				key := u.GetStateRequest.Key

				// get state from ledger
				value, err := stub.GetState(key)
				if err != nil {
					return shim.Error(err.Error())
				}

				// since nil is not a valid string type in go
				// we pass an empty string as Value in case the value returend by GetState is nil
				var valueString string
				if value == nil {
					valueString = ""
				} else {
					valueString = string(value)
				}

				// create and send putStateResponse gRPC message for chaincode_proxy
				wrapperMsg := &grpcpb.ChaincodeWrapperMessage{
					MessageOneof: &grpcpb.ChaincodeWrapperMessage_GetStateResponse{
						GetStateResponse: &grpcpb.GetStateResponse{
							Value: valueString,
						},
					},
				}
				err = stream.Send(wrapperMsg)
				if err != nil {
					return shim.Error(err.Error())
				}
			}
		case *grpcpb.ChaincodeProxyMessage_PutStateRequest:
			{
				// read key and value of PutStateRequest gRPC message from chaincode_proxy
				key := u.PutStateRequest.Key
				value := u.PutStateRequest.Value

				// put state on ledger
				err = stub.PutState(key, []byte(value))
				if err != nil {
					return shim.Error(err.Error())
				}

				// create and send putStateResponse gRPC message for chaincode_proxy
				wrapperMsg := &grpcpb.ChaincodeWrapperMessage{
					MessageOneof: &grpcpb.ChaincodeWrapperMessage_PutStateResponse{
						PutStateResponse: &grpcpb.PutStateResponse{
							Acknowledgement: "OK",
						},
					},
				}
				err := stream.Send(wrapperMsg)
				if err != nil {
					return shim.Error(err.Error())
				}
			}
		}
	}
}
