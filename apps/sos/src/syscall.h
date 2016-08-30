#include "sel4/types.h"
#include "vm/addrspace.h"

#define SOS_SYS_BRK 50

#define SOS_SYS_USLEEP 126
#define SOS_SYS_TIMESTAMP 127
#define SOS_SYS_WRITE 1

int sos_sleep(int msec, seL4_CPtr reply_cap);
uint32_t sos_brk(long newbreak, pageDirectory * pd, region * heap);

void sos_wake(int* id, void* data);
