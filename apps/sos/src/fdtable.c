#include "fdtable.h"
#include <stdlib.h>
#include <string.h>

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>


void register_device(void* newDev, char* str, read_t read, int maxReaders, write_t write, int maxWriters){
	int i = 0;
	while(i < MAX_IO_DEVICES && fdDevices[i].device != NULL){
		if(strcmp(fdDevices[i].name, str)){
			return;
		}
		i++;
	}
	if(i == MAX_IO_DEVICES){
		return;
	}
	dprintf(0, "Registering device %d\n", i);
	fdDevices[i].device = newDev;
	fdDevices[i].name = malloc(sizeof(char)*DEVNAME_LEN);
	strncpy(fdDevices[i].name, str, DEVNAME_LEN);
	fdDevices[i].read = read;
	fdDevices[i].maxReaders = maxReaders;
	fdDevices[i].maxWriters = maxWriters;
	fdDevices[i].write = write;
}

void unregister_device(char* name){
	for(int i = 0; i < MAX_IO_DEVICES; i++){
		if(strcmp(name, fdDevices[i].name)){
			fdDevices[i].device = NULL;
			fdDevices[i].name = NULL;
			fdDevices[i].read = NULL;
			fdDevices[i].write = NULL;
			break;
		}
	}	
}

int open_device(char* name, fdnode* fdtable, fd_mode mode){
	for(int i = 0; i < MAX_IO_DEVICES; i++){
		if(strcmp(name, fdDevices[i].name)){
			if(mode == fdReadOnly || mode == fdReadWrite){
				if(fdDevices[i].readers == fdDevices[i].maxReaders){
					dprintf(0,"Read Open attempted on full device %x\n", i);
					return 0;
				}
			}
			if(mode == fdWriteOnly || mode == fdReadWrite){
				if(fdDevices[i].writers == fdDevices[i].maxWriters){
					dprintf(0,"Write open attempted on full device %x\n", i);
					return 0;
				}			
			}
			for(int j = 3; j < MAX_FILES; j++){
				if(fdtable[j].file == NULL){
					fdtable[j].file = (seL4_Word)&fdDevices[i];
					fdtable[j].type = fdDev;
					fdtable[j].permissions = mode;
					if(mode == fdReadOnly || mode == fdReadWrite) fdDevices[i].readers++;
					if(mode == fdWriteOnly || mode == fdReadWrite) fdDevices[i].writers++;
					return j;
				}
			}	
			return -1;
		}
	}
	dprintf(0, "No Device Found on Open!\n");
	return 0;
}

void close_device(fdnode* fdtable, int index){
		fdnode* fd = &(fdtable[index]);
		fdDevice* device = fd->file;
		fd->type = 0;
		if(fd->permissions == fdReadOnly || fd->permissions == fdReadWrite) device->readers--;
		if(fd->permissions == fdWriteOnly || fd->permissions == fdReadWrite) device->writers--;
	
		fd->file = NULL;
		fd->permissions = 0;
		fd->type = 0; 
}
