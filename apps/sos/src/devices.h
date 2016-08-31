#ifndef __DEVICES_H
#define __DEVICES_H

#include "serial/serial.h"
#include "sel4/types.h"
#include "vm/addrspace.h"

#define IO_BUFFER_LENGTH 8096

int serial_read(struct serial* serial, char* buff, int len, seL4_CPtr reply, region* shared_region);
void read_finish(char* buff, int len, seL4_CPtr reply, region *shared_region);

int serial_write(struct serial* serial, char* buff, int len);
void serial_callback(struct serial * serial, char c);

#endif
