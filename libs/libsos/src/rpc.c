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

void rpc_call_data(seL4_MessageInfo_t tag, void *vData, size_t count, seL4_Word endpoint){
    seL4_SetMR(1, vData);
    seL4_SetMR(2, count);
    rpc_call(tag, endpoint);
}

void rpc_call_mr(seL4_MessageInfo_t tag, seL4_Word endpoint) {
    rpc_call(tag, endpoint);
}

