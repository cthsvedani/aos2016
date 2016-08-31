#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "sel4/types.h"
#include "vm/addrspace.h"
#include "fdtable.h"

#define SOS_SYS_BRK 50

#define SOS_SYS_USLEEP 126
#define SOS_SYS_TIMESTAMP 127
#define SOS_SYS_OPEN 2
#define SOS_SYS_CLOSE 3

#define SOS_SYS_SLEEP 126
#define SOS_SYS_WRITE 114
#define SOS_SYS_READ 112

int sos_sleep(int msec, seL4_CPtr reply_cap);
uint32_t sos_brk(long newbreak, pageDirectory * pd, region * heap);
int sos_open(char* path, fdnode* fdtable, fd_mode mode);
void sos_close(fdnode* fdtable, int index);

void sos_wake(int* id, void* data);

#endif
