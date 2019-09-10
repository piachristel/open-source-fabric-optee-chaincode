#ifndef CHAINCODE_TEE_REE_COMMUNICATION_H
#define CHAINCODE_TEE_REE_COMMUNICATION_H

#include <stdint.h>

/* max size for strings is inclusive null terminator */
#define FCT_SIZE 20
#define KEY_SIZE 20
#define VAL_SIZE 20
#define ARG_SIZE 20
#define ARGS_NUMBER 10
#define RESPONSE_SIZE 100
#define ACK_SIZE 20

#define INVOCATION_RESPONSE 0
#define GET_STATE_REQUEST 1
#define PUT_STATE_REQUEST  2
#define ERROR 100

/* used for InvokeCommand API */
#define TA_CHAINCODE_CMD_INIT_INVOKE 0 
#define TA_CHAINCODE_CMD_RESUME_INVOKE 1

struct key_value {
	char key[KEY_SIZE];
	char value[VAL_SIZE];
};

struct arguments {
	char arguments[ARGS_NUMBER][ARG_SIZE];
};

struct invocation_response {
	char execution_response[RESPONSE_SIZE];
};

struct acknowledgement {
	char acknowledgement[ACK_SIZE];
};

#endif /* CHAINCODE_TEE_REE_COMMUNICATION_H */
