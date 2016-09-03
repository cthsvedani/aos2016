#include "fsystem.h"
#include "nfs/nfs.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

extern fhandle_t mnt_point;

void fsystemStart(){
	assert(MAX_NFS_REQUESTS < 256); //We store some data in the higher bits of our nfs token.
								 //Make sure it's seperate from our fs_request index!
	register_timer(NFS_TIME, timeout, NULL);
	memset(fs_req, 0, MAX_NFS_REQUESTS*sizeof(fs_request*));	
}

void timeout(uint32_t id, void* data){
	id = 0; //Supress warnings
	data = NULL; 
	register_timer(NFS_TIME, timeout, NULL);
	nfs_timeout();
}
int fs_next_index(){
	for(int i = 0; i < MAX_NFS_REQUESTS; i++){
		if(fs_req[i] == NULL){
			fs_req[i] = malloc(sizeof(fs_request));
			if(!fs_req[i]){
				return -1;
			}
		return i;
		}
	}
	return -1;
}

void fs_free_index(int i){
	if(i >= 0 && i < MAX_NFS_REQUESTS && fs_req[i] != NULL){
		free(fs_req[i]);
		fs_req[i] = NULL;
	}
}

void fs_stat(char* kbuff, shared_region* stat_region, seL4_CPtr reply){
	dprintf(0, "In fs_stat\n");
	uint32_t* token = malloc(sizeof(uint32_t));
	if(!token){
		conditional_panic(1,"fs_stat\n");
	}
	*token = fs_next_index();
	if(*token < 0){
		dprintf(0, "Nope\n");
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->kbuff = kbuff;
	fs_req[*token]->s_region = stat_region;
	
	nfs_lookup(&mnt_point ,kbuff, fs_stat_complete, (uintptr_t)token);
}

void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t *fattr){
	dprintf(0, "In fs_stat_complete\n");
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
		dprintf(0, "Failed with code %d\n", status);
		seL4_SetMR(0, -1);
	}
	seL4_Send(fs_req[*i]->reply, tag);
	cspace_delete_cap(cur_cspace, fs_req[*i]->reply);
	free(fs_req[*i]->kbuff);
	fs_free_index(*i);
	free(i);
}

void fs_getDirEnt(char* kbuff, shared_region * name_region, seL4_CPtr reply, int position, size_t n){
	dprintf(0, "In fs_getDirEnt\n");
	uint32_t* token = malloc(sizeof(uint32_t));
	if(!token){
		conditional_panic(1,"fs_getDirEnt\n");
	}
	*token = fs_next_index();
	if(*token < 0){
		dprintf(0, "Nope\n");
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->kbuff = kbuff;
	fs_req[*token]->s_region = name_region;
	fs_req[*token]->fdIndex = position;
	fs_req[*token]->fdtable = (fdnode*)n; //Smuggling ints in pointers to give compilers heart attacks.
	
	nfs_readdir(&mnt_point , 0, fs_getDirEnt_complete, (uintptr_t)token);

}

void fs_getDirEnt_complete(uintptr_t token, nfs_stat_t status, int num_files, char* file_names[], nfscookie_t nfscookie){
	uint32_t t = *(uint32_t*)token;
	t = (t << 24) >> 24; //Drop the upper 24 bits
	fs_request* req = fs_req[t];
	uint32_t files = ((*(uint32_t*)token) >> 8) + num_files;
	if(nfscookie != 0 && files < req->fdIndex){
		t += files << 8;
		*(uint32_t*)token = t;		
		nfs_readdir(&mnt_point, nfscookie, fs_getDirEnt_complete, (uintptr_t)token);
		return;
	}
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	if(nfscookie == 0 && files < req->fdIndex){
		seL4_SetMR(0, -1);	
	}
	else{
		files = (files - num_files) + (size_t)req->fdtable; //!!!
		strncpy(req->kbuff, file_names[files], 1024);
		put_to_shared_region(req->s_region, req->kbuff);
		seL4_SetMR(0,0);	
	}
	seL4_Send(req->reply, tag);
	cspace_delete_cap(cur_cspace, req->reply);
	free(req->kbuff);
	free_shared_region_list(req->s_region);
	fs_free_index(t);
	free((void*)token);
}
