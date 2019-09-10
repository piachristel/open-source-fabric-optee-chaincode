/*
 * examples that helped me to write my code (no copy paste):
 * https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_server.cc
 * https://github.com/linaro-swg/optee_examples/tree/master/hello_world
 * https://github.com/protocolbuffers/protobuf/blob/2e27bb70961ba15c87435dc90dddefdf46089320/src/google/protobuf/util/field_mask_util_test.cc
 * https://grpc.github.io/grpc/cpp/
 */

/*
 * used conventions:
 * https://www.kernel.org/doc/html/latest/process/coding-style.html
 * (since optee uses that as stated in https://buildmedia.readthedocs.org/media/pdf/optee/latest/optee.pdf)
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <csignal>

#include <tee_client_api.h>

#include <chaincode_tee_ree_communication.h>

#include <grpcpp/grpcpp.h>
#include "invocation.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using invocation::ChaincodeProxyMessage;
using invocation::ChaincodeWrapperMessage;
using invocation::GetStateRequest;
using invocation::PutStateRequest;
using invocation::InvocationResponse;
using invocation::Invocation;

/* declare static functions */
static void run_server();
void cleanup();

/* server logic */
class InvocationImpl final : public Invocation::Service
{
	/* write the uuid received from the chaincode_wrapper to the shared memory */
	static void set_uuid(ChaincodeWrapperMessage *wrapper_msg, TEEC_UUID *uuid)
	{
		std::string chaincode_uuid = wrapper_msg->invocation_request().chaincode_uuid();
		uuid->clockSeqAndNode[0] = chaincode_uuid[8];
		uuid->clockSeqAndNode[1] = chaincode_uuid[9];
		uuid->clockSeqAndNode[2] = chaincode_uuid[10];
		uuid->clockSeqAndNode[3] = chaincode_uuid[11];
		uuid->clockSeqAndNode[4] = chaincode_uuid[12];
		uuid->clockSeqAndNode[5] = chaincode_uuid[13];
		uuid->clockSeqAndNode[6] = chaincode_uuid[14];
		uuid->clockSeqAndNode[7] = chaincode_uuid[15];
		uuid->timeLow = (chaincode_uuid[0] << 24) | (chaincode_uuid[1] << 16) | (chaincode_uuid[2] << 8) | (chaincode_uuid[3]);
		uuid->timeMid = (chaincode_uuid[4] << 8) | (chaincode_uuid[5]);
		uuid->timeHiAndVersion = (chaincode_uuid[6] << 8) | (chaincode_uuid[7]);
	}

	/* write the arguments received from the chaincode_wrapper to the shared memory */
	static bool set_args(ChaincodeWrapperMessage *wrapper_msg, struct arguments *arguments_data)
	{
		if(wrapper_msg->invocation_request().arguments_size() > ARGS_NUMBER) {
			return false;
		}
		for (int i = 0; i < wrapper_msg->invocation_request().arguments_size(); i++) {
			std::string arg = wrapper_msg->invocation_request().arguments(i); 
			if(arg.length()+1 > ARG_SIZE) {
				return false;
			}
			memset(arguments_data->arguments[i], 0, ARG_SIZE);
			memcpy(arguments_data->arguments[i], arg.c_str(), arg.length()); 
		}
		return true;
	}

	/* write the function_name received from the chaincode_wrapper to the shared buffer */
	static bool set_function(ChaincodeWrapperMessage *wrapper_msg, char *function, TEEC_Operation *op)
	{
		if(wrapper_msg->invocation_request().function_name().length()+1 > FCT_SIZE) {
			return false;
		}
		memcpy(function, wrapper_msg->invocation_request().function_name().c_str(), wrapper_msg->invocation_request().function_name().length()); 
		op->params[0].tmpref.buffer = function;
		op->params[0].tmpref.size = FCT_SIZE; 
		return true;
	}

	/* forward the invocation response received from the chaincode to the chaincode_wrapper */
	static bool handle_invocation_response(TEEC_Operation *op, ServerReaderWriter<ChaincodeProxyMessage, ChaincodeWrapperMessage> *stream )
	{
		/* read result from shared memory */
		struct invocation_response *invocation_response_data;
		invocation_response_data = (struct invocation_response *)(op->params[2].memref.parent->buffer);
		char response[RESPONSE_SIZE]; 
		memset(response, 0, RESPONSE_SIZE);
		memcpy(response, invocation_response_data->execution_response, strlen(invocation_response_data->execution_response)); 

		/* create and send final result gRPC message to chaincode_wrapper */
		ChaincodeProxyMessage proxy_msg;
		InvocationResponse* invocation_response = new InvocationResponse();
		invocation_response->set_execution_response(response);
		proxy_msg.set_allocated_invocation_response(invocation_response);
		return stream->Write(proxy_msg);
	}

	/* 
	* forward the get state request of the chaincode to the chaincode_wrapper 
	* wait for the get state response of the chaincode_wrapper
	* write this response to the shared memory 
	*/
	static bool handle_get_state_request(TEEC_Operation *op, ServerReaderWriter<ChaincodeProxyMessage, ChaincodeWrapperMessage> *stream, TEEC_SharedMemory *data)
	{
		/* read key for GetState from shared memory */
		struct key_value *key_value_data;
		key_value_data = (struct key_value *)(op->params[2].memref.parent->buffer);
		char key[KEY_SIZE];
		memset(key, 0, KEY_SIZE);
		memcpy(key, key_value_data->key, strlen(key_value_data->key)); 

		/* create and send getState gRPC message to chaincode_wrapper */
		ChaincodeProxyMessage proxy_msg;
		GetStateRequest* get_state_request = new GetStateRequest();
		get_state_request->set_key(key);
		proxy_msg.set_allocated_get_state_request(get_state_request);
		if(!stream->Write(proxy_msg)) {
			return false;
		}

		/* read value of getState grpc message from chaincode_wrapper */
		ChaincodeWrapperMessage wrapper_msg;
		if(!stream->Read(&wrapper_msg)) {
			return false;
		}
		std::string get_state_response = wrapper_msg.get_state_response().value();

		/* write value of putState to shared memory */
		if(get_state_response.length() + 1 > VAL_SIZE) {
			return false;
		}
		key_value_data = (struct key_value *)data->buffer;
		memset(key_value_data, 0, sizeof(key_value));
		memcpy(key_value_data->value, get_state_response.c_str(), get_state_response.length());
		return true;
	}

	/* 
	* forward the put state request of the chaincode to the chaincode_wrapper 
	* wait for the put state response of the chaincode_wrapper
	* write this response to the shared memory 
	*/
	static bool handle_put_state_request(TEEC_Operation *op, ServerReaderWriter<ChaincodeProxyMessage, ChaincodeWrapperMessage> *stream,  TEEC_SharedMemory *data)
	{
		/* read key and value for PutState from shared memory */
		struct key_value *key_value_data;
		key_value_data = (struct key_value *)(op->params[2].memref.parent->buffer);
		char key[KEY_SIZE];
		char value[VAL_SIZE];
		memset(key, 0, KEY_SIZE); 
		memcpy(key, key_value_data->key, strlen(key_value_data->key));
		memset(value, 0, VAL_SIZE); 
		memcpy(value, key_value_data->value, strlen(key_value_data->value));
	
		/* create and send putState gRPC message to chaincode_wrapper*/
		ChaincodeProxyMessage proxy_msg;
		PutStateRequest* put_state_request = new PutStateRequest();
		put_state_request->set_key(key);
		put_state_request->set_value(value);
		proxy_msg.set_allocated_put_state_request(put_state_request);
		if(!stream->Write(proxy_msg)) {
			return false;
		}					

		/* read acknowledgement of putState gRPC message from chaincode_wrapper */
		ChaincodeWrapperMessage wrapper_msg;
		if(!stream->Read(&wrapper_msg)) {
			return false;
		}
		std::string put_state_response = wrapper_msg.put_state_response().acknowledgement();

		/* write ack of putState to shared memory */
		if(put_state_response.length() + 1 > ACK_SIZE) {
			return false;
		}
		struct acknowledgement *acknowledgement_data = (struct acknowledgement *)data->buffer;
		memset(acknowledgement_data, 0, sizeof(acknowledgement));
		memcpy(acknowledgement_data->acknowledgement, put_state_response.c_str(), put_state_response.length());

		return true;
	}

	Status TransactionInvocation(ServerContext *context, ServerReaderWriter<ChaincodeProxyMessage, ChaincodeWrapperMessage> *stream) override
	{
		/* declare variables used within one gRPC chaincode_wrapper - chaincode_proxy stream */
		TEEC_Context ctx;
		TEEC_Result res = TEEC_SUCCESS;
		TEEC_Session sess;
		TEEC_Operation op;
		TEEC_UUID uuid; 
		TEEC_SharedMemory data;
		uint32_t err_origin;
		bool success = true;
		
		/* read invocation message */
		ChaincodeWrapperMessage wrapper_msg;
		success = stream->Read(&wrapper_msg);
		if(!success) {
			return Status(grpc::StatusCode::UNKNOWN, "");  
		}

		/* set UUID */
		set_uuid(&wrapper_msg, &uuid);

		res = TEEC_InitializeContext(NULL, &ctx);
		if (res != TEEC_SUCCESS) {
			return Status(grpc::StatusCode::UNKNOWN, "");
		}
		
		/* open session towards chaincode */
		res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
		if (res != TEEC_SUCCESS) {
			TEEC_FinalizeContext(&ctx);
			return Status(grpc::StatusCode::UNKNOWN, "failed to open session"); 
		}
		
		/*
		* define the types of the parameters
	 	* - first parameter (shared memory): 
		* contains the function to execute inside the chaincode
	 	* - second paramter (parameter value uint32_t): 
		* used to pass the type of message (final result, getState or putState) from the chaincode to this chaincode_proxy
		* type uint32_t allows switch statement inside chaincode_proxy
	 	* - third parameter (shared memory): 
		* used to transfer data (arguments, key for GetState, key-value for PutState, result of GetState, result PutState, final result)
	 	* - fourth parameter: 
		* unused
	 	*/
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT, TEEC_MEMREF_WHOLE, TEEC_NONE);

		/* allocate shared memory used for second paramter */
		size_t structure_sizes[] = { sizeof(struct key_value), sizeof(struct acknowledgement), sizeof(struct invocation_response), sizeof(struct arguments) };
		data.size = *std::max_element(std::begin(structure_sizes), std::end(structure_sizes));
		data.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
		res = TEEC_AllocateSharedMemory(&ctx, &data);
		if (res != TEEC_SUCCESS) {
			TEEC_CloseSession(&sess);
			return Status(grpc::StatusCode::UNKNOWN, "failed to allocate shared memory");
		}
		op.params[2].memref.parent = &data;
		memset(data.buffer, 0, data.size);

		/* set first parameter: function name */
		char *function;
		function = (char *)calloc(FCT_SIZE, sizeof(char)); 
		success = set_function(&wrapper_msg, function, &op);
		if(!success){
			TEEC_CloseSession(&sess);
			return Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid size of function"); 
		}

		/* set third parameter: arguments */
		struct arguments *arguments_data;
		arguments_data  = (struct arguments *)data.buffer; 
		success = set_args(&wrapper_msg, arguments_data);
		if(!success){
			goto cleanup;
		}

		/* 
		* call chaincode inside OP-TEE
		* TA_CHAINCODE_CMD_INIT_INVOKE indicates that any old context inside the chaincode TA must be cleared 
		*/
		res = TEEC_InvokeCommand(&sess, TA_CHAINCODE_CMD_INIT_INVOKE, &op, &err_origin);
		if (res != TEEC_SUCCESS) {
			goto cleanup;
		}
		
		/*
		* read the return type stored in the second paramter by the chaincode: invocation response, get state request or put state request
		* - in case of an invocation response, the chaincode_proxy forwards this response to the chaincode_wrapper, leaves the loop and closes the stream
		* - in case of a get state request, the chaincode_proxy forwards the request to the chaincode_wrapper, 
		*   waits for the response, writes the response to the shared memory, invokes the chaincode again and restarts the loop
		* - in case of a put state request, the chaincode_proxy forwards the request to the chaincode_wrapper,
		*   waits for the response, writes the response to the shared memory, invokes the chaincode again and restarts the loop 
		*/
		while(true) {
			switch(op.params[1].value.a) {
				case INVOCATION_RESPONSE: {
					success = handle_invocation_response(&op, stream);
					goto cleanup;
				}
				case GET_STATE_REQUEST: {
					success = handle_get_state_request(&op, stream, &data);
					if(!success){
						goto cleanup;
					}
					break;
				}
				case PUT_STATE_REQUEST: {
					success = handle_put_state_request(&op, stream, &data);
					if(!success){
						goto cleanup;
					}
					break;
				}
				case ERROR: {
					success = false;
					goto cleanup;
				}
				default: {	
					success = false;
					goto cleanup;
				}
			}

			/* 
			* call chaincode inside OP-TEE
			* TA_CHAINCODE_CMD_RESUME_INVOKE indicates that the chaincode execution in the chaincode TA must be resumed based on the stored context 
			*/
			res = TEEC_InvokeCommand(&sess, TA_CHAINCODE_CMD_RESUME_INVOKE, &op, &err_origin);
			if (res != TEEC_SUCCESS) {
				goto cleanup;
			}
		}

	cleanup: 
		free(function);
		TEEC_ReleaseSharedMemory(&data);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		if(res != TEEC_SUCCESS || !success){
			/* signalize that stream has unsuccessfully finished, returns the boolean false in read/write process of chaincode_wrapper */
			return Status(grpc::StatusCode::UNKNOWN, "");
		}
		/* signalize that invocation has been finished */	
		return Status::OK;
	}
};

void cleanup(int signum)
{
	exit(0);
}

static void run_server()
{
	/* create server, add listening port and register service */
	std::string server_address("0.0.0.0:50051");
	InvocationImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	/* start the server */
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "...chaincode_proxy is listening at " << server_address << std::endl;

	/* 
	* let gRPC server stream run and handle incoming transaction invocation
	* until it gets shutted down or killed 
	*/
	server->Wait();

}

int main(void)
{
	/* catch Ctrl+C keyboard event to cleanup (kill gRPC server) */
	signal(SIGINT, cleanup);
	
	/*
	* start the gRPC server stream and
	* handle incoming transaction invocation
	*/
	run_server();

	return 0;
}

