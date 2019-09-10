/*
Copyright IBM Corp. All Rights Reserved.
SPDX-License-Identifier: Apache-2.0
*/

// examples that helped me to write my code (no copy paste)
// https://github.com/hyperledger/fabric/blob/release-1.4/examples/chaincode/go/example02/cmd/main.go

package main

import (
	"fmt"

	"github.com/hyperledger/fabric/core/chaincode/shim"
	"github.com/hyperledger/fabric/examples/chaincode/go/chaincode_wrapper"
)

func main() {
	err := shim.Start(new(chaincode_wrapper.ChaincodeWrapper))
	if err != nil {
		fmt.Printf("Failed to start chaincode wrapper with error: %s", err)
	}
}
