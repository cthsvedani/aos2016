#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

struct addrspace * as_create(void);
int as_define_region(struct addrspace *as, int flags);
void as_destroy();

struct region* new_region(struct addrspace* as);
struct region* find_region(struct addrspace* as);

void free_region_list( struct region* head);
void free_page_table(void );

typedef struct addrspace {
    region *regions;
} addrspace;

typedef struct region_t {
    seL4_Word vbase;
    seL4_Word size;
    seL4_Word flags;
    region_t *next;

} region;

#endif
