#ifndef __SOS_FSYSTEM_H_
#define __SOS_FSYSTEM_H_

#include "sel4/types.h"
#include "fdtable.h"
#include "vm/addrspace.h"
#include "clock/clock.h"
#include "nfs/nfs.h"

#define MAX_PROCESSES 10
#define MAX_NFS_REQUESTS 20 
#define NFS_TIME 100000
#define MAX_REQUEST_SIZE 4096
#define WRITE_MULTI 1	
#define MAX_FILE_SIZE 1024

typedef struct request{
	seL4_CPtr reply;
	char* kbuff;
	fdnode* fdtable;
	int fdIndex;
	shared_region* s_region;
	int data;
    int count;
    int read;
    int swapping;
}fs_request;

typedef struct event_getDirEnt{
    uintptr_t token;
    nfs_stat_t status;
    int num_files;
    nfscookie_t nfscookie;
    char **file_names;
}evt_getDirEnt;

typedef struct event_read{
    uintptr_t token;
    nfs_stat_t status;
    int count;
    void *data;
}evt_read;

typedef struct event_write{
    uintptr_t token;
    nfs_stat_t status;
    int count;
}evt_write;

typedef struct {
  int		 st_type;    /* file type */
  int		 st_fmode;   /* access mode */
  unsigned	 st_size;    /* file size in bytes */
  long		 st_crtime;   /* file creation time (ms since booting) */
  long		 st_actime;   /* file last access (open) time (ms since booting) */
} stat_t;

fs_request * fs_req[MAX_NFS_REQUESTS];

int fs_next_index();
void fs_free_index();

void fsystemStart();
void timeout(uint32_t id, void* data);

void fs_open(char* buff,fdnode* fdtable, fd_mode mode,seL4_CPtr reply);
void fs_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);
void fs_open_create(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);

void fs_close(fdnode* fdtable, int index);

void fs_write(fdnode* f_ptr, shared_region* reg, size_t count, seL4_CPtr reply, int offset, int swapping);
void fs_write_complete(uintptr_t token, nfs_stat_t status, fattr_t * fattr, int count);
void evt_write_complete(void *data);

void fs_stat(char* name, shared_region* stat_region, seL4_CPtr reply);
void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t * fattr);

void fs_getDirEnt(char* kbuff, shared_region* name_region, seL4_CPtr reply ,int position, size_t n);
void fs_getDirEnt_complete(uintptr_t token, nfs_stat_t status, int num_files, char* file_names[], nfscookie_t nfscookie);
void evt_getDirEnt_complete(void* data);

void fs_read(fdnode *f_ptr, shared_region *stat_region, seL4_CPtr reply, size_t count, int offset, int swapping);
void fs_read_complete(uintptr_t token, nfs_stat_t status, fattr_t *fattr, int count, void *data);
void evt_read_complete(void *data);
#endif
