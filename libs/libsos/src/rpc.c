#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sos/rpc.h>

void rpc_call(seL4_MessageInfo_t tag, seL4_Word endpoint) {
    seL4_SetTag(tag);
    seL4_Call(endpoint, tag);
}

void rpc_call_data(seL4_MessageInfo_t tag, seL4_Word syscall_num, void* buf){
    
}

void rpc_call_mr(seL4_MessageInfo_t tag, seL4_Word endpoint) {
    rpc_call(tag, endpoint);
}

