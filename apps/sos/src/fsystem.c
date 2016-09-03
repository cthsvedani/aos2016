#include "fsystem.h"
#include "nfs/nfs.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

extern fhandle_t mnt_point;

void fsystemStart(){
	register_timer(NFS_TIME, timeout, NULL);	
}

void timeout(uint32_t id, void* data){
	id = 0; //Supress warnings
	data = NULL; 
	register_timer(NFS_TIME, timeout, NULL);
	nfs_timeout();
}
int fs_next_index(){
	for(int i = 0; i < MAX_PROCESSES; i++){
		if(fs_req[i] == NULL){
			fs_req[i] = malloc(sizeof(fs_request));
			if(!fs_req[i]){
				return -1;
			}
		}
		return i;
	}
	return -1;
}

void fs_free_index(int i){
	if(fs_req[i] != NULL){
		free(fs_req[i]);
		fs_req[i] = NULL;
	}
}

void fs_stat(char* kbuff, shared_region* stat_region, seL4_CPtr reply){
	uint32_t* token = malloc(sizeof(uint32_t));
	if(!token){
		conditional_panic(1,"fs_stat\n");
	}
	*token = fs_next_index();
	if(*token == 0){
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->kbuff = kbuff;
	fs_req[*token]->s_region = stat_region;
	
	nfs_lookup(&mnt_point ,kbuff, fs_stat_complete, (uintptr_t)token);
}

void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t *fattr){
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	uint32_t* i = (uint32_t*)token;
	if(status == NFS_OK){
		fs_request * req = fs_req[*i];
		stat_t* ret = (stat_t*)req->kbuff;
		ret->st_type = fattr->type;	
		ret->st_fmode = fattr->mode;
		ret->st_size = fattr->size;
		uint64_t time = time_stamp()/1000;
		ret->st_ctime = time;
		ret->st_atime = time;
		put_to_shared_region(req->s_region, (char*) ret);
		seL4_SetMR(0, 0);
	}
	else{
		seL4_SetMR(0, -1);
	}
	seL4_Send(fs_req[*i]->reply, tag);
	cspace_delete_cap(cur_cspace, fs_req[*i]->reply);
	free(fs_req[*i]->kbuff);
	fs_free_index(*i);
	free(i);
}
