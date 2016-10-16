#include "fsystem.h"
#include "nfs/nfs.h"
#include "pagefile.h"
#include "syscall.h"
#include "events.h"

#include <sys/stat.h>

#define verbose 1 
#include <sys/debug.h>
#include <sys/panic.h>

extern fhandle_t mnt_point;
extern int jump;

void fsystemStart(){
	assert(MAX_NFS_REQUESTS < 256); //We store some data in the higher bits of our nfs token.
								 //Make sure it's seperate from our fs_request index!
	register_timer(NFS_TIME, timeout, NULL);

    register_event(GETDIRENT_COMPLETE, evt_getDirEnt_complete);
    register_event(READ_COMPLETE, evt_read_complete);
    register_event(WRITE_COMPLETE, evt_write_complete);
    register_event(STAT_COMPLETE, evt_stat_complete);

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

void fs_read(fdnode *f_ptr, shared_region *stat_region, seL4_CPtr reply, size_t count, int offset, int swapping){
    dprintf(1, "In fs_read, count is %d \n", count);
    uint32_t *token = malloc(sizeof(uint32_t));
    if(!token) {
		if(reply != 0) reply_failed(reply);
    }
	*token = fs_next_index();
	if(*token < 0){
		if(reply != 0) reply_failed(reply);
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->s_region = stat_region;
    fs_req[*token]->fdtable = f_ptr;
    fs_req[*token]->count = count;
    fs_req[*token]->read = 0;
    fs_req[*token]->swapping = swapping;

	count = (count < 4096 ) ? count : 4096;	
    rpc_stat_t  ret = nfs_read((fhandle_t*)f_ptr->file, f_ptr->offset, count, fs_read_complete, (uintptr_t)token);
    if(ret != RPC_OK) {
		dprintf(0, "fs_read failed with code %d\n", ret);
    }
}

void evt_read_complete(void *evt){
    evt_read *event = (evt_read*)evt;
    uintptr_t token = event->token;
    nfs_stat_t status = event->status;
    int count = event->count;
    void *data = event->data;

    free(event);
	dprintf(1, "In fs_read_complete, read %d bytes \n", count);
	uint32_t* i = (uint32_t*)token;
    fs_request * req = fs_req[*i];
	seL4_MessageInfo_t tag;
	if(status == NFS_OK){
        req->fdtable->offset += count;
        put_to_shared_region_n(&(req->s_region), (char*) data, count,req->swapping);
        free(data);
        req->count -= count;
        req->read += count;
        if(count != 0 && req->count > 0 ) {
            nfs_read((fhandle_t *)req->fdtable->file, req->fdtable->offset, (req->count > MAX_REQUEST_SIZE) ? MAX_REQUEST_SIZE : req->count, fs_read_complete, (uintptr_t)token);
            return;
        }
		if(req->reply != 0){
			tag = seL4_MessageInfo_new(0,0,0,1);
			seL4_SetMR(0, req->read);
		} else {
			free_shared_region_list(req->s_region, 1);
			fs_free_index(*i);
			free(i);
			jump = 1;
			return;
		}
	} else if (req->reply != 0){
        free(data);
		dprintf(0, "Failed with code %d\n", status);
		tag = seL4_MessageInfo_new(0,0,0,1);
		seL4_SetMR(0, -1);
	}
	if(req->reply != 0){
		seL4_Send(req->reply, tag);
		cspace_delete_cap(cur_cspace, req->reply);
	}	
    dprintf(1, "Read Done with %d Bytes\n", req->read);
	free_shared_region_list(req->s_region, 0);
	fs_free_index(*i);
	free(i);
}

void fs_read_complete(uintptr_t token, nfs_stat_t status, fattr_t *fattr, int count, void *data){
    evt_read *event = malloc(sizeof(evt_read));
    event->token = token;
    event->status = status;
    event->count = count;
    
    void *buf = malloc(count * sizeof(char));
    memcpy(buf, data, count);

    event->data = buf;
    emit(READ_COMPLETE,(void*)event);
}

void fs_stat(char* kbuff, shared_region* stat_region, seL4_CPtr reply){
	dprintf(0, "In fs_stat\n");
	uint32_t* token = malloc(sizeof(uint32_t));
	if(!token){
		reply_failed(reply);
	}
	*token = fs_next_index();
	if(*token < 0){
		reply_failed(reply);
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->kbuff = kbuff;
	fs_req[*token]->s_region = stat_region;
	
	nfs_lookup(&mnt_point ,kbuff, fs_stat_complete, (uintptr_t)token);
}

void evt_stat_complete(void* data){
	dprintf(0, "In evt_stat_complete\n");
    evt_stat *event = (evt_stat*)data;
    stat_t *ret = event->ret;
    fs_request *req = event->req;
    uint32_t* i = event->i;
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
    put_to_shared_region(req->s_region, (char*) ret);
	dprintf(0, "In evt_stat_complete, put to shared completed\n");
    dprintf(0, "evt fs_stat_complete, ret %d, req is %d, req->reply is %d\n", ret, req, req->reply);
    seL4_SetMR(0, 0);
    seL4_Send(req->reply, tag);
    cspace_delete_cap(cur_cspace, req->reply);
    free(req->kbuff);
    free(event->ret);
    free(event);
    fs_free_index(*i);
    free(i);
}

void fs_stat_complete(uintptr_t token, nfs_stat_t status, fhandle_t *fh, fattr_t *fattr){
	dprintf(0, "In fs_stat_complete\n");
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	uint32_t* i = (uint32_t*)token;
	if(status == NFS_OK){
		fs_request * req = fs_req[*i];
		stat_t* ret = malloc(sizeof(stat_t));
		if(S_ISREG(fattr->mode)){
			ret->st_type = 1;
		} else {
			ret->st_type = 2;
		}
		ret->st_fmode = fattr->mode;
		ret->st_size = fattr->size;
		uint64_t time = time_stamp()/1000;
		ret->st_crtime = time;
		ret->st_actime = time;

        //need to emit event since we can page_fault
        evt_stat *event = malloc(sizeof(evt_stat));
        event->i = i;
        event->ret = ret;
        event->req = req;
        dprintf(0, "fs_stat_complete, ret %d, req is %d, req->reply is %d\n", ret, req, req->reply);
        emit(STAT_COMPLETE, (void*)event);
	} else {
		dprintf(0, "Failed with code %d\n", status);
		seL4_SetMR(0, -1);
        seL4_Send(fs_req[*i]->reply, tag);
        cspace_delete_cap(cur_cspace, fs_req[*i]->reply);
        free(fs_req[*i]->kbuff);
        fs_free_index(*i);
        free(i);
	}
}

void fs_getDirEnt(char* kbuff, shared_region * name_region, seL4_CPtr reply, int position, size_t n){
	dprintf(0, "In fs_getDirEnt\n");
	uint32_t* token = malloc(sizeof(uint32_t));
	if(!token){
		reply_failed(reply);
		conditional_panic(1,"fs_getDirEnt\n");
	}
	*token = fs_next_index();
	if(*token == -1){
        free(token);
		reply_failed(reply);
		dprintf(0, "Nope\n");
		return;
	}
	fs_req[*token]->reply = reply;
	fs_req[*token]->kbuff = kbuff;
	fs_req[*token]->s_region = name_region;
	fs_req[*token]->fdIndex = position;
	fs_req[*token]->fdtable = (fdnode*)n; //Smuggling ints in pointers to give compilers heart attacks.
    
	rpc_stat_t ret = nfs_readdir(&mnt_point , 0, fs_getDirEnt_complete, (uintptr_t)token);
    if(ret != RPC_OK) {
		dprintf(0, "fs_getDirEnt failed with code %d\n", ret);
    } else {
        dprintf(1, "fs_getDirent returned with code %d\n", ret);
    }
}

void evt_getDirEnt_complete(void* data){
    evt_getDirEnt *event = (evt_getDirEnt*) data;
    uintptr_t token = event->token;
    nfs_stat_t status = event->status;
    int num_files = event->num_files;
    char **file_names = event->file_names;
    nfscookie_t nfscookie = event->nfscookie;
    free(event);
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
	} else if(files == req->fdIndex) {
        seL4_SetMR(0, 0);
    } else {
		files = req->fdIndex - (files - num_files);//!!!
        assert(files < num_files);
        size_t n = ((size_t)req->fdtable < 1024) ? (size_t)req->fdtable : 1024;
		strncpy(req->kbuff, file_names[files], n);
        if(n == 1024) {
            req->kbuff[1023] = '\0';
        }
		put_to_shared_region(req->s_region, req->kbuff);
		seL4_SetMR(0,strlen(req->kbuff));	
	}
	seL4_Send(req->reply, tag);
	cspace_delete_cap(cur_cspace, req->reply);
	free(req->kbuff);
	free_shared_region_list(req->s_region, 0);
	fs_free_index(t);
	free((uint32_t*)token);
    free(file_names);
}
static int strcnt(char *str) {
    for(int i = 0; i < MAX_FILE_SIZE; i++) {
        if(str[i] == '\0') {
            return i+1;
        }
    } 
    return -1;
}

void fs_getDirEnt_complete(uintptr_t token, nfs_stat_t status, int num_files, char* file_names[], nfscookie_t nfscookie){
    char **file_names2 = malloc(sizeof(char*) * num_files);
    if(!file_names2) {
        panic("Nomem: fs_getDirEnt");
    }
    for(int i = 0; i < num_files; i++) {
        int n = strcnt(file_names[i]);
        file_names2[i] = malloc(sizeof(char) * n);
        if(!file_names2[i]) {
            panic("Nomem: fs_getDirEnt");
        }
        strncpy(file_names2[i], file_names[i], n);
    }

    evt_getDirEnt *event = malloc(sizeof(evt_getDirEnt));
    event->token = token;
    event->status = status;
    event->num_files = num_files;
    event->file_names = file_names2;
    event->nfscookie = nfscookie;
    emit(GETDIRENT_COMPLETE, (void*)event);
}

void fs_open(char* buff, fdnode* fdtable, fd_mode mode,seL4_CPtr reply){
	uint32_t* index = malloc(sizeof(uint32_t));
	if(!index){
		dprintf(0,"malloc failed in fs_open\n");
		reply_failed(reply);
		return;
	}

	*index = fs_next_index();
	if(*index == -1){
		free(index);
		dprintf(0,"No free NFS Indicies!\n");
		reply_failed(reply);
		return;
	}
	int j;
	for(j = 3; j < MAX_FILES; j++){
		if(fdtable[j].file == 0){ //This is safe in single threaded code, 
			fdtable[j].file = 1;  //Might explode if multi-threaded.
			fdtable[j].type = fdFile;
			fdtable[j].permissions = mode;
			fs_req[*index]->fdIndex = j;
			break;
		}
	}
	if(j == MAX_FILES){
		fs_free_index(*index);
		free(index);
		reply_failed(reply);
	}
	

	fs_req[*index]->fdtable = fdtable;
	fs_req[*index]->kbuff = buff;
	fs_req[*index]->reply = reply; 
	nfs_lookup(&mnt_point, buff, fs_open_complete, (uintptr_t)index);
}

void fs_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	uint32_t * i = (uint32_t*) token;
	fs_request* req = fs_req[*i];
	if(status == NFSERR_NOENT){
		sattr_t attr;
		attr.mode = (6 << 6) + 6;
		attr.uid = 0;
		attr.gid = 0;
		attr.size = 0;
		attr.atime.seconds = -1;
		attr.atime.useconds = -1;
		attr.mtime.seconds = -1;
		attr.mtime.useconds = -1;
	
		nfs_create(&mnt_point, req->kbuff, &attr, fs_open_create, token);
		return;
	}
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	fdnode* fd = &(req->fdtable[req->fdIndex]);
	if(status == NFS_OK){
		int ok = 0;
		//Compare permissions, save to fdtable, go home.
		if(fd->permissions == fdReadOnly || fd->permissions == fdReadWrite){
			ok = (fattr->mode & 4) ? 1 : 0;
		}
		if(fd->permissions == fdWriteOnly || fd->permissions == fdReadWrite){
			ok = (fattr->mode & 2) ? 1 : 0;
		}
		if(ok){
			dprintf(0,"Permissions okay\n");
			fd->file = (seL4_Word)malloc(sizeof(fhandle_t));
			memcpy((void*)fd->file, fh, sizeof(fhandle_t));
			seL4_SetMR(0, req->fdIndex);
			dprintf(0, "Returning %d\n", req->fdIndex);
		}
		else{
			dprintf(0, "Permission rejection\n");
			fd->file = 0;
			seL4_SetMR(0, -1);
		}
	}
	else{
		dprintf(0, "Something happened\n");
		fd->file = 0;
		seL4_SetMR(0, -1);
	}
	seL4_Send(req->reply, tag);
	cspace_delete_cap(cur_cspace, req->reply);
	free(req->kbuff);
	fs_free_index(*i);
	free(i);	
}

void fs_open_create(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	uint32_t * i = (uint32_t*) token;
	fs_request* req = fs_req[*i];
	fdnode* fd = &(req->fdtable[req->fdIndex]);
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	if(status == NFS_OK){
			fd->file = (seL4_Word)malloc(sizeof(fhandle_t));
			memcpy((void*)fd->file, fh, sizeof(fhandle_t));
			seL4_SetMR(0, req->fdIndex);
	}
	else{
		seL4_SetMR(0, -1);
		fd->file = 0;	
	}
	seL4_Send(req->reply, tag);
	cspace_delete_cap(cur_cspace, req->reply);
	free(req->kbuff);
	fs_free_index(*i);
	free(i);
}
//Index validation must be done before calling fs_close!
void fs_close(fdnode* fdtable, int index){
	//Since the NFS is stateless, all we need to do is forget the fhandle, and clear our own bookkeeping.
	//If filesystem caching is implemented, we should flush the cache here.
	fdtable[index].type = 0;
	free((void*)fdtable[index].file);
	fdtable[index].file = 0;
	fdtable[index].offset = 0;
	fdtable[index].permissions = 0;
}

void fs_write(fdnode* f_ptr, shared_region* reg, size_t count, seL4_CPtr reply, int offset, int swapping){
	uint32_t * i =	malloc(sizeof(uint32_t));
	if(i == NULL){
        dprintf(1, "in fs_write swapping, malloc failed\n");
		free_shared_region_list(reg, swapping);
		if(reply != 0) reply_failed(reply);
		return;	
	}
	
	*i = fs_next_index();
	if(*i == -1){
		free(i);
        dprintf(1, "in fs_write swapping, no free fs_req\n");
		free_shared_region_list(reg, swapping);
		if(reply != 0) reply_failed(reply);
		return;
	}

    dprintf(1, "in fs_write swapping %d, f_ptr %d, count %d, reply %d, offset %d, i %d\n", swapping, f_ptr, count, reply, offset/4096, *i);
	fs_req[*i]->reply = reply;
	fs_req[*i]->fdIndex = count;
	fs_req[*i]->fdtable = f_ptr;	
	fs_req[*i]->data = 0;
	fs_req[*i]->count = 0;
	fs_req[*i]->kbuff = (char*)reg;
    fs_req[*i]->swapping = swapping;
    
    //do multiple writes for swapping (all writes on same page)
    if(fs_req[*i]->swapping) {
        for(int j = 0; j < WRITE_MULTI; j++){
            size_t size = reg->size;
            if(size > 1024){
                size = 1024;
            }
            seL4_Word sos_vaddr;
            sos_vaddr = reg->user_addr;
            if(nfs_write((fhandle_t*)f_ptr->file, f_ptr->offset, size, (void*)sos_vaddr,
                             fs_write_complete, (uintptr_t)i) == RPC_OK){
                reg->size -= size;
                reg->user_addr += size;
                if(reg->size <= 0){	
                    reg = reg->next;
                }
                f_ptr->offset += size;
                fs_req[*i]->count++;
                fs_req[*i]->s_region = reg;
                if(!reg) break;
            } else {
                dprintf(0, "NFS_WRITE_FAILED\n");
            }
        }
    //currently, multiple writes for spanning different pages is not working
    } else {
        size_t size = reg->size;
        if(size > 1024){
            size = 1024;
        }
        seL4_Word sos_vaddr;
        sos_vaddr = get_user_translation(reg->user_addr, reg->user_pd);
        dprintf(0, "NFS_WRITE CALLED\n");
        if(nfs_write((fhandle_t*)f_ptr->file, f_ptr->offset, size, (void*)sos_vaddr,
                         fs_write_complete, (uintptr_t)i) == RPC_OK){
            reg->size -= size;
            reg->user_addr += size;
            if(reg->size <= 0){	
                reg = reg->next;
            }
            f_ptr->offset += size;
            fs_req[*i]->count++;
            fs_req[*i]->s_region = reg;
            if(!reg) return;
        } else {
            dprintf(0, "NFS_WRITE_FAILED\n");
        }
    }
}

/* Preconditions: The write was succesfull and we are not swapping */
void evt_write_complete(void *data) {
    evt_write *event = (evt_write*)data;
    uintptr_t token = event->token;
    nfs_stat_t status = event->status;
    int count = event->count;
    free(event);

	uint32_t * i = (uint32_t*) token;
	fs_request * req = fs_req[*i];
	fdnode * fd = req->fdtable;
    dprintf(0, "NFS_WRITE_COMPLETE, swapping is %d \n", req->swapping);
	seL4_MessageInfo_t tag;	

    if(req->s_region != NULL){
        size_t size = req->s_region->size;
        if(size > 1024){
            size = 1024;
        }
        seL4_Word sos_vaddr = get_user_translation(req->s_region->user_addr, req->s_region->user_pd);
        nfs_write((fhandle_t*)(fd->file), fd->offset, size,
                (void*)sos_vaddr, fs_write_complete, (uintptr_t)i);
        req->s_region->size -= size;
        req->s_region->user_addr += size;
        if(req->s_region->size <= 0){
            req->s_region = req->s_region->next;
        }
        fd->offset += size;	
        return;	
    } else {
        req->count--;
        if(req->count != 0){				
            return;
        }
    }

    tag = seL4_MessageInfo_new(0,0,0,1);
    seL4_SetMR(0, req->data);
    seL4_CPtr reply = req->reply;
    free_shared_region_list((shared_region*)fs_req[*i]->kbuff, fs_req[*i]->swapping);
    fs_free_index(*i);
    free(i);
    seL4_Send(reply, tag);
    cspace_delete_cap(cur_cspace, reply);
}

void fs_write_complete(uintptr_t token, nfs_stat_t status, fattr_t * fattr, int count){
	uint32_t * i = (uint32_t*) token;
	fs_request * req = fs_req[*i];
	fdnode * fd = req->fdtable;
    dprintf(1, "in FS_WRITE_COMPLETE, swapping is %d \n", req->swapping);
	seL4_MessageInfo_t tag;	
	req->data += count;

    if(status != NFS_OK && req->reply) {
		tag = seL4_MessageInfo_new(0,0,0,1);
		seL4_SetMR(0, -1);
        seL4_CPtr reply = fs_req[*i]->reply;
        free_shared_region_list((shared_region*)fs_req[*i]->kbuff, fs_req[*i]->swapping);
        fs_free_index(*i);
        free(i);
		seL4_Send(reply, tag);
		cspace_delete_cap(cur_cspace, reply);
    } else if(status != NFS_OK) {
        panic("swapping failed");
        return;
    }

    if(req->swapping) {
        req->count--;
        if(req->count != 0){				
            return;
        }

        free_shared_region_list((shared_region*)fs_req[*i]->kbuff, fs_req[*i]->swapping);
        fs_free_index(*i);
        free(i);
        jump = 1;
        return;
    } else {
        evt_write *event = malloc(sizeof(evt_write));
        if(!event){
            panic("Nomem: fs_write_complete");
        }
        event->token = token;
        event->status = status;
        event->count = count;
        emit(WRITE_COMPLETE,(void*)event);
    }
}
