#include "fdtable.h"
#include <stdlib.h>
#include <string.h>

void register_device(void* newDev, char* str, read_t read, int maxReaders, write_t write, int maxWriters){
	int i = 0;
	while(i < MAX_IO_DEVICES && fdDevices[i].device == NULL){
		if(strcmp(fdDevices[i].name, str)){
			return;
		}
		i++;
	}
	if(i == MAX_IO_DEVICES){
		return;
	}
	fdDevices[i].device = newDev;
	strcpy(str, fdDevices[i].name);
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
					return 0;
				}
			}
			if(mode == fdWriteOnly || mode == fdReadWrite){
				if(fdDevices[i].writers == fdDevices[i].maxWriters){
					return 0;
				}			
			}
			for(int j = 0; j < MAX_FILES; j++){
				if(fdtable[j].file == NULL){
					fdtable[j].file = (seL4_Word*)&fdDevices[i];
					fdtable[j].type = fdDev;
					fdtable[j].permissions = mode;
					if(mode == fdReadOnly || mode == fdReadWrite) fdDevices[i].readers++;
					if(mode == fdWriteOnly || mode == fdReadWrite) fdDevices[i].writers++;
					return i;
				}
			}	
		}
	}
	return 0;
}

void close_device(fdnode* fdtable, int index){
	
}
