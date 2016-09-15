#ifndef _PAGEFILE_H_
#define _PAGEFILE_H_
#include "fdtable.h"
#include "proc.h"
#include "frametable.h"
#include <nfs/nfs.h>

#define PAGEFILE_NAME "pagefile"
#define PAGEFILE_PAGES 1000000

typedef struct pf_free_region {
    int offset;
    uint32_t size;
    struct pf_free_region *next;
} pf_region;

//pagefile bookkeeping
int pf_init();
int pf_free(int offset);
int pf_get_next_offset();

//pagefile operation

void pf_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);
void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr);

void pf_return();

void pf_fault_in();
void pf_write_out(int pfIndex, frame* fr);
void pf_write_out_complete(uintptr_t token, nfs_stat_t status, fattr_t * fattr, int count);
frame* clock(int force);

#endif
