#include "fsystem.h"
#include "nfs/nfs.h"

#define verbose 5

#include <sys/debug.h>

struct	ip_addr ip;

void fsystemStart(){
	dprintf(0, "Starting File System... \t");
	ip.addr = ipaddr_addr(SERVER);
	rpc_stat_t stat = nfs_init(&ip);
	if(stat == RPC_OK){
		dprintf(0, "Success!\n");
	}
	else{
		dprintf(0, "Recieved error code %d\n",  stat);
	}
	register_timer(NFS_TIME, timeout, NULL);	
}

void timeout(int* id, void* data){
	id = NULL; //Supress warnings
	data = NULL; 
	register_timer(NFS_TIME, timeout, NULL);
	nfs_timeout();
}
