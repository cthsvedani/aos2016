#ifndef _FILE_DESCRIPTOR_TABLE_H
#define _FILE_DESCRIPTOR_TABLE_H

#include "serial/serial.h"
#include "sel4/types.h"
#include "vm/addrspace.h"

#define MAX_FILES 32
#define MAX_IO_DEVICES 32
#define DEVNAME_LEN 1024

typedef enum {
	fdDev = 0,
	fdFile = 1,
} fd_type;

typedef enum{
	fdReadOnly = 0,
	fdWriteOnly,
	fdReadWrite,
} fd_mode;

typedef int (*read_t)(void* device, char* buffer, int len, seL4_CPtr reply, shared_region * shared_region);
typedef int (*write_t)(void* device, char* buffer, int len);

typedef struct fddev{
	void* device;
	char* name;
	read_t read;
	write_t write;
	int readers, writers;
	int maxReaders, maxWriters;
} fdDevice;

fdDevice fdDevices[MAX_IO_DEVICES];

typedef struct {
	fd_mode permissions;
	fd_type type;
	seL4_Word file;//fdDev* if device, fhandle_t* if NFS			
    int offset;
} fdnode;

void register_device(void* newDev, char* str, read_t read, int maxReaders, write_t write, int maxWriters);
void unregister_device(char* name);

int open_device(char* name, fdnode* fdtable, fd_mode mode);
void close_device(fdnode* fdtable, int index);

shared_region * get_shared_region(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd, fd_mode mode);
int pt_ckptr(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd, fd_mode mode);
#endif
