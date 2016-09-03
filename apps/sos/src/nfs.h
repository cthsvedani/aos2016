#ifndef _NFS_H_
#define _NFS_H_
#include <nfs/nfs.h>

void sos_nfs_connect(const struct ip_addr* ip_addr);
void sos_register_timeout();
void sos_nfs_timeout(uint32_t id, void* data);
#endif
