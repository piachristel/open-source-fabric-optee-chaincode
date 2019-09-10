#include <string.h>
#include <chaincode_library.h>

void init_context(struct chaincode_ctx *ctx, TEE_Param params[4])
{
	/* stores function in context */ 
	TEE_MemFill(ctx->chaincode_fct, 0, FCT_SIZE);
	TEE_MemMove(ctx->chaincode_fct,  params[0].memref.buffer, strlen( params[0].memref.buffer)); 
	
	/* stores args in context */
	struct arguments *args_data = (struct arguments *)params[2].memref.buffer;
	TEE_MemFill(&(ctx->chaincode_args), 0, sizeof(struct arguments));
	TEE_MemMove(&(ctx->chaincode_args), args_data, sizeof(struct arguments)); 
}

TEE_Result cleanup(TEE_Param params[4])
{
	params[1].value.a = ERROR;
	return TEE_SUCCESS;
}

TEE_Result write_response(TEE_Param params[4], char *response)
{
	if(strlen(response)+1 > RESPONSE_SIZE) {
		cleanup(params);
	}
	struct invocation_response *invocation_response_data = (struct invocation_response *)params[2].memref.buffer;
	TEE_MemFill(invocation_response_data->execution_response, 0, RESPONSE_SIZE);
	TEE_MemMove(invocation_response_data->execution_response, response, RESPONSE_SIZE);
	params[1].value.a = INVOCATION_RESPONSE;
    return TEE_SUCCESS;
}

void read_get_state(TEE_Param params[4], char *val)
{
	struct key_value *key_value_data = (struct key_value *)params[2].memref.buffer;
	TEE_MemFill(val, 0, VAL_SIZE); 
	TEE_MemMove(val, key_value_data->value, strlen(key_value_data->value)); 
}

void read_put_state(TEE_Param params[4], char *ack) 
{
	struct acknowledgement *ack_data = (struct acknowledgement *)params[2].memref.buffer;
	TEE_MemFill(ack, 0, ACK_SIZE);
	TEE_MemMove(ack, ack_data->acknowledgement, strlen(ack_data->acknowledgement));
}

TEE_Result write_put_state(TEE_Param params[4], char *key, char *val)
{
	/* cleanup if key or value does not fit into key_value structure */
	if(strlen(key)+1 > KEY_SIZE || strlen(val)+1 > VAL_SIZE) {
		cleanup(params);
	}

	struct key_value *key_value_data = (struct key_value *)params[2].memref.buffer;
	TEE_MemFill(key_value_data->key, 0, KEY_SIZE);
	TEE_MemMove(key_value_data->key, key, KEY_SIZE); 
	TEE_MemFill(key_value_data->value, 0, VAL_SIZE);
	TEE_MemMove(key_value_data->value, val, VAL_SIZE); 

	params[1].value.a = PUT_STATE_REQUEST;
    return TEE_SUCCESS;
}

TEE_Result write_get_state(TEE_Param params[4], char *key)
{
	/* cleanup if key does not fit into key_value structure */
	if(strlen(key)+1 > KEY_SIZE) {
		return cleanup(params);
	}

	struct key_value *key_value_data = (struct key_value *)params[2].memref.buffer;
	TEE_MemFill(key_value_data->key, 0, KEY_SIZE);
	TEE_MemMove(key_value_data->key, key, KEY_SIZE); 

    params[1].value.a = GET_STATE_REQUEST;
    return TEE_SUCCESS;
}