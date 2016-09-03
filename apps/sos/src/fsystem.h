#ifndef __SOS_FSYSTEM_H_
#define __SOS_FSYSTEM_H_

#include "sel4/types.h"
#include "fdtable.h"
#include "vm/addrspace.h"
#include "clock/clock.h"
#include "nfs/nfs.h"

#define MAX_PROCESSES 10
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

fs_request * fs_req[MAX_PROCESSES];

int fs_next_index();
void fs_free_index();

void fsystemStart();
void timeout(uint32_t id, void* data);

int fs_open(char* buff, fdnode* fdtable, seL4_CPtr reply);
void fs_open_complete(int index);

void fs_stat(char* name, shared_region* stat_region, seL4_CPtr reply);
void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t * fattr);


#endif
