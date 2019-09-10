#ifndef CHAINCODE_LIBRARY_H
#define CHAINCODE_LIBRARY_H

#include <tee_internal_api.h>
#include <chaincode_tee_ree_communication.h>

/* general structure for storing the context */
struct chaincode_ctx {
	char chaincode_fct[FCT_SIZE];
	int chaincode_fct_state;
	struct arguments chaincode_args;
};

/* general methods for communication with chaincode_proxy */
void init_context(struct chaincode_ctx *ctx, TEE_Param params[4]);
TEE_Result cleanup(TEE_Param params[4]);
TEE_Result write_response(TEE_Param params[4], char *response);
void read_get_state(TEE_Param params[4], char *val);
void read_put_state(TEE_Param params[4], char *ack);
TEE_Result write_put_state(TEE_Param params[4], char *key, char *val);
TEE_Result write_get_state(TEE_Param params[4], char *key);

#endif /* CHAINCODE_LIBRARY_H */