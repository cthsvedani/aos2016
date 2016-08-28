#ifndef _RPC_H_
#define _RPC_H_

/**
 * Free the given rpc object
 */
void* rpc_free(void *addr);

/**
 * invoke the server
 */
int rpc_call_server();

typedef struct rpc_buffer_s {
    void *data;
    //uint32_t count;
} rpc_buffer_t;

typedef struct rpc_client_state_s {
    //msginfo_t minfo;
    //void* obj[RPC_MAX_TRACKED_OBJS];
    //uint32_t num_obj;

    //bool skip_reply;
    //ENDPT reply;

    void *userptr;
} rpc_client_state_t;

typedef struct binding_object {
    void* client_vaddr;
    void* server_vaddr;
};
#endif
