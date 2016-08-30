#ifndef __DEVICES_H
#define __DEVICES_H

#include "serial/serial.h"
#include "sel4/types.h"

#define IO_BUFFER_LENGTH 8096

int serial_read(struct serial* serial, char* buff, int len, seL4_CPtr reply);
int serial_write(struct serial* serial, char* buff, int len);

void serial_callback(struct serial * serial, char c);

#endif
