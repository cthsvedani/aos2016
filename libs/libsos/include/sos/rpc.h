#ifndef _RPC_H_
#define _RPC_H_

void rpc_call(seL4_MessageInfo_t tag, seL4_Word endpoint);
void rpc_call_data(seL4_MessageInfo_t tag, seL4_Word syscall_num, void* buf);
void rpc_call_mr(seL4_MessageInfo_t tag, seL4_Word endpoint);
#endif
