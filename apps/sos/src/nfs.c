#include "nfs.h"
#include <nfs/nfs.h>
#include "timer.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>
void sos_nfs_connect(const struct ip_addr* ip_addr) {
    sos_register_timeout();
    nfs_init(ip_addr);
}

void sos_register_timeout() {
	seL4_CPtr* data = malloc(sizeof(seL4_CPtr));
    register_timer(1000000, (timer_callback_t)sos_nfs_timeout, (void*)data);
}

void sos_nfs_timeout(uint32_t id, void* data) {
    dprintf(0, "in nfs timeout \n");
    register_timer(1000000, (timer_callback_t)sos_nfs_timeout, data);
    /*nfs_timeout();*/
}
