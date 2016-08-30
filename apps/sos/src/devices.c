#include "devices.h"

char serialbuffer[IO_BUFFER_LENGTH];
int serialReadIndex = 0;
int serialWriteIndex = 0;
int newlineIndex = -1;
int buffLen = 0;

void* finish_func = NULL;
char* finish_buff = NULL;
int finish_len = NULL;
seL4_CPtr finish_reply = NULL;

void setup_finish(char* buff, int len, seL4_CPtr reply){
	finish_func = read_finish;
	finish_buff = buff;
	finish_len = len;
	finish_reply = reply;
}

int serial_read(struct serial* serial, char* buff, int len, seL4_CPtr reply){
	if(buffLen < len && newlineIndex == -1){
		setup_finish(buff, len, reply);
	}
	
	read_finish(buff, len, seL4_CPtr reply);

	return 0;
}

int serial_write(struct serial* serial, char* buff, int len){
	return 0;
}

void read_finish(char* buff, int len, seL4_CPtr reply){
	if(buffLen < len && newlineIndex == -1) return;
	if(buffLen > len){
		if(serialReadIndex < serialWriteIndex){
			memcpy(buff, serialbuffer + serialReadIndex, len);
			serialReadIndex += len;
			finish_func = NULL;
			return;
		}
		else{
			//Fuck around with circular Buffer
		}
	}

	if(newlineIndex != -1){
		int offset = newlineIndex - readIndex
		if(offset > 0){
			memcpy(buff, serialbuffer + serialReadIndex, offset);
			finish_func = NULL;
			return;
		}
		else{
			//Fuck around with circular buffer
		}
	}
	else{
	
	}		
}

void serial_callback(struct serial * serial, char c){
	if(serialWriteIndex + 1 != serialReadIndex){
		serialbuffer[serialWriteIndex] = c;
		if(c == '\n') newlineIndex = serialWriteIndex;
		serialWriteIndex++;
		buffLen++;
		if(serialWriteIndex == IO_BUFFER_LENGTH){
			serialWriteIndex = 0;	
		}
	}
	if(finish_func){
		finish_func(finish_buff, finish_len, finish_reply);
	}
}
