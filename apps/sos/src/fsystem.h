#ifndef __SOS_FSYSTEM_H_
#define __SOS_FSYSTEM_H_

#include "sel4/types.h"
#include "fdtable.h"
#include "vm/addrspace.h"
#include "clock/clock.h"
#include "nfs/nfs.h"

#define MAX_PROCESSES 10
#define MAX_NFS_REQUESTS MAX_PROCESSES
#define NFS_TIME 100000

typedef struct request{
	seL4_CPtr reply;
	char* kbuff;
	fdnode* fdtable;
	int fdIndex;
	shared_region* s_region;
}fs_request;

typedef struct {
  int		 st_type;    /* file type */
  int		 st_fmode;   /* access mode */
  unsigned	 st_size;    /* file size in bytes */
  long		 st_ctime;   /* file creation time (ms since booting) */
  long		 st_atime;   /* file last access (open) time (ms since booting) */
} stat_t;

fs_request * fs_req[MAX_NFS_REQUESTS];

int fs_next_index();
void fs_free_index();

void fsystemStart();
void timeout(uint32_t id, void* data);

void fs_open(char* buff,fdnode* fdtable, fd_mode mode,seL4_CPtr reply);
void fs_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);
void fs_open_create(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);

void fs_stat(char* name, shared_region* stat_region, seL4_CPtr reply);
void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t * fattr);

void fs_getDirEnt(char* kbuff, shared_region* name_region, seL4_CPtr reply ,int position, size_t n);
void fs_getDirEnt_complete(uintptr_t token, nfs_stat_t status, int num_files, char* file_names[], nfscookie_t nfscookie);

void fs_read(fhandle_t *handle, shared_region *stat_region, seL4_CPtr reply, size_t count, int offset);
void fs_read_complete(uintptr_t token, nfs_stat_t status, fattr_t *fattr, int count, void *data);
#endif
