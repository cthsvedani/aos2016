#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sel4/sel4.h>

//global counters for MR and cap
uint32_t cur_mr;
uint32_t cur_cp;
seL4_CPtr endpoint;
seL4_MessageInfo_t reply;

int rpc_call_server() {
    //call server, mr and size set
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, cur_cp, cur_mr);
    reply = seL4_Call(endpoint, tag);
    return 0;
}
