#ifndef _PAGEFILE_H_
#define _PAGEFILE_H_
#include "fdtable.h"
#include "proc.h"
#include <nfs/nfs.h>

#define PAGEFILE_NAME "pagefile"

int pf_write_out(frame* out);
int pf_init();
void pf_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);
void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);

void pf_fault_in();
void clock();

#endif
