/*
 * examples that helped me to write my code (no copy paste)
 * https://github.com/linaro-swg/optee_examples/tree/master/hello_world
 */

/*
 * used conventions:
 * https://www.kernel.org/doc/html/latest/process/coding-style.html
 * (since optee uses that as stated in https://buildmedia.readthedocs.org/media/pdf/optee/latest/optee.pdf)
 */

#include <string.h>
#include <stdlib.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <coffee_tracking_chaincode.h>
#include <chaincode_library.h>
#include <chaincode_tee_ree_communication.h>

/* declare coffee tracking chaincode specific static functions used in this file only */
static TEE_Result query(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result query_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result query_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result create(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result create_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result create_put_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result create_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result add(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result add_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result add_put_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);
static TEE_Result add_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4]);

TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		cleanup(params);

	/* unused */
	(void)&params;

	/* initialize session context state with 0 */
	struct chaincode_ctx *ctx = calloc(1, sizeof(*ctx));
	ctx->chaincode_fct_state = 0;
	*sess_ctx = ctx;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	free(sess_ctx);
}

static TEE_Result add_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* read ack of PutState */
	char ack[ACK_SIZE];
	read_put_state(params, ack);
	if(strcmp(ack, "OK") != 0) { 
		return cleanup(params);
	}

	/* send result */
	char response[RESPONSE_SIZE] = "OK";
	return write_response(params, response);
}

static TEE_Result add_put_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* read value of GetState */ 
	char val_string[VAL_SIZE];  
	read_get_state(params, val_string);
	
	/* throw an error if person does not exit */
	if(strcmp(val_string, "") == 0){ 
		return cleanup(params);
	}
	char *ptr;
	unsigned long val = strtoul(val_string, &ptr, 10);

	/* PutState */
	unsigned long consumed_coffees = strtoul(ctx->chaincode_args.arguments[1], &ptr, 10);
	consumed_coffees += val;
	char consumed_coffees_string[VAL_SIZE];  
	TEE_MemFill(consumed_coffees_string, 0, VAL_SIZE);
	snprintf(consumed_coffees_string, VAL_SIZE-1, "%lu", consumed_coffees); /* unsigend long long takes 8 bytes, so it will fit into the char array of size VAL_SIZE */
	return write_put_state(params, ctx->chaincode_args.arguments[0], consumed_coffees_string);
}

static TEE_Result add_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* GetState */
	return write_get_state(params, ctx->chaincode_args.arguments[0]);
}

/*
 * Add function
 * arguments
 * first: ctx->chaincode_args.arguments[0] - person
 * second: ctx->chaincode_args.arguments[1] - number of coffees
 * 
 * increases the consumed coffees of person by the number of coffess
 */
static TEE_Result add(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	FMSG("increasing number of consumed coffees...");
	switch(ctx->chaincode_fct_state) {
		case ADD_GET_STATE: {
			TEE_Result res = add_get_state(ctx, param_types, params);

			/* set context to next state */
			ctx->chaincode_fct_state = 1;
			return res;
		}
		case ADD_PUT_STATE: {
			TEE_Result res = add_put_state(ctx, param_types, params);

			/* set context to next state */
			ctx->chaincode_fct_state = 2;
			return res;
		}	
		case ADD_WRITE_RESPONSE: {
			TEE_Result res = add_write_response(ctx, param_types, params);

			/* reset context to initial state */
			ctx->chaincode_fct_state = 0;
			return res;
		}
		default: {
			cleanup(params);
		}
	}
}

static TEE_Result create_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* read ack of PutState */
	char ack[ACK_SIZE];
	read_put_state(params, ack);
	if(strcmp(ack, "OK") != 0) {
		return cleanup(params);
	}

	/* send result */
	char response[RESPONSE_SIZE] = "OK";
	return write_response(params, response);
}

static TEE_Result create_put_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* read value of GetState */ 
	char val[VAL_SIZE];
	read_get_state(params, val);

	/* throw an error if person already exists */
	if(strcmp(val, "") != 0){  
		return cleanup(params);
	}

	/* PutState */
	char *ptr;
	unsigned long consumed_coffees = strtoul(ctx->chaincode_args.arguments[1], &ptr, 10);
	char consumed_coffees_string[VAL_SIZE]; 
	TEE_MemFill(consumed_coffees_string, 0, VAL_SIZE);
	snprintf(consumed_coffees_string, VAL_SIZE-1, "%lu", consumed_coffees); /* unsigend long long takes 8 bytes, so it will fit into the char array of size VAL_SIZE */
	return write_put_state(params, ctx->chaincode_args.arguments[0], consumed_coffees_string);
}

static TEE_Result create_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* GetState */
	return write_get_state(params, ctx->chaincode_args.arguments[0]);
}

/*
 * Create function
 * arguments
 * first: ctx->chaincode_args.arguments[0] - person
 * second: ctx->chaincode_args.arguments[1] - number of coffees
 * 
 * creates a new person with consumed coffees set to number of coffees
 * 
 * if person exists already on the ledger, an error is thrown
 */
static TEE_Result create(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
        FMSG("creating new account and storing initial number of consumed coffees...");
		switch(ctx->chaincode_fct_state) {
			case CREATE_GET_STATE: {
				TEE_Result res = create_get_state(ctx, param_types, params);

				/* set context to next state */
				ctx->chaincode_fct_state = 1;
				return res;
			}
			case CREATE_PUT_STATE: {
				TEE_Result res = create_put_state(ctx, param_types, params);

				/* set context to next state */
				ctx->chaincode_fct_state = 2;
				return res;
			}	
			case CREATE_WRITE_RESPONSE: {
				TEE_Result res = create_write_response(ctx, param_types, params);

				/* reset context to initial state */
				ctx->chaincode_fct_state = 0;
				return res;
			}
			default: {
				cleanup(params);
			}
	}
}

static TEE_Result query_write_response(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* read value of GetState */ 
	char val[VAL_SIZE];
	read_get_state(params, val);
	
	/* throw an error if person does not exit */
	if(strcmp(val, "") == 0){  
		return cleanup(params);
	}

	/* send result */
	char *person = "person: ";
	char *coffee = ", number of consumed coffees: ";
	char response[RESPONSE_SIZE];  
	TEE_MemFill(response, 0, RESPONSE_SIZE);
	TEE_MemMove(response, person, strlen(person));
	TEE_MemMove(response+strlen(person), ctx->chaincode_args.arguments[0], strlen(ctx->chaincode_args.arguments[0]));
	TEE_MemMove(response+strlen(person)+strlen(ctx->chaincode_args.arguments[0]), coffee, strlen(coffee));
	TEE_MemMove(response+strlen(person)+strlen(ctx->chaincode_args.arguments[0])+strlen(coffee), val, strlen(val));
	return write_response(params, response);
}

static TEE_Result query_get_state(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
	/* GetState */
	return write_get_state(params, ctx->chaincode_args.arguments[0]);
}

/*
 * Query function
 * arguments
 * first: ctx->chaincode_args.arguments[0] - person
 * 
 * queries the number of consumed coffees of person
 */
static TEE_Result query(struct chaincode_ctx *ctx, uint32_t param_types, TEE_Param params[4])
{
        FMSG("query entitiy person on ledger...");
		switch(ctx->chaincode_fct_state) {
		case QUERY_GET_STATE: {
			TEE_Result res = query_get_state(ctx, param_types, params);

			/* set context to next state */
			ctx->chaincode_fct_state = 1;
			return res;
		}
		case QUERY_WRITE_RESPONSE: {
			TEE_Result res = query_write_response(ctx, param_types, params);

			/* reset context to initial state */
			ctx->chaincode_fct_state = 0;
			return res;
		}
		default: {
			cleanup(params);
		}
	}
        return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, 
			TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
												TEE_PARAM_TYPE_VALUE_OUTPUT,
                                                TEE_PARAM_TYPE_MEMREF_INOUT,
                                            	TEE_PARAM_TYPE_NONE);
	/* check parameter types */
	if (param_types != exp_param_types) {
		cleanup(params); 
	}

	struct chaincode_ctx *ctx = (struct chaincode_ctx *)sess_ctx;
 
	/* 
	 * initialize chaincode context in case of initial function call (TA_CHAINCODE_CMD_INIT_INVOKE)
	 * nothing needs to be initialized in case of cmd_id == TA_CHAINCODE_CMD_RESUME_INVOKE
	 */
	if(cmd_id == TA_CHAINCODE_CMD_INIT_INVOKE) {
		init_context(ctx, params);
	}

	/* call the function */ 
	if (strcmp(ctx->chaincode_fct, "create") == 0) {
		return create(ctx, param_types, params);
	}
	else if (strcmp(ctx->chaincode_fct, "add") == 0) {
		return add(ctx, param_types, params);
	}
	else if (strcmp(ctx->chaincode_fct, "query") == 0) {
		return query(ctx, param_types, params);
	}
	else {	
		cleanup(params);
	}
}
