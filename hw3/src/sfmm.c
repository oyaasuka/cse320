/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

#define RSIZE 8
#define DSIZE 16
#define M 32/*minimal size of a block*/
#define PSIZE 4096




/*Help functions*/
size_t pack(size_t size,size_t alloc,size_t prev_alloc);

size_t get(void* bp);
void put(size_t* bp,size_t val);

size_t get_size(void* bp);
size_t get_alloc(void* bp);
size_t get_prev_alloc(void*bp);

size_t* get_hdp(void* bp);
size_t* get_ftp(void* bp);


void* get_prev_blkp(void* bp);
void* get_next_blkp(void* bp);

void* coalesce(void *bp);
void* firstFit_blkp(size_t alloc_size, int index);
void* split(void* free_blk, size_t asize);
void insert_free_blk(void *free_blkp);
void* place(void *alloc_blkp, size_t size);
void extend_empty_block(void* extended_pp);
int if_heap_full();
int validate_argument(void* pp);
size_t find_freelist(size_t asize);




void *sf_malloc(size_t size) {
    size_t asize;
    int i;/*proper location of free lists*/
    void* free_blkp;

    /*first call sf_malloc, then initialize the heap*/
    if(sf_mem_start()==sf_mem_end()){

        void* prologue_blkp = sf_mem_grow();
        put(get_hdp(prologue_blkp), pack(32,1,1));
        put(get_ftp(prologue_blkp), pack(32,1,1)^sf_magic());


        void* epilogue_blkp = (void*)((char*)sf_mem_end()-8);
        put(epilogue_blkp, pack(0,1,0));

        void* remainder = (void*)((char*)prologue_blkp+32);
        put(get_hdp(remainder),pack(4048,0,1));
        put(get_ftp(remainder),pack(4048,0,1)^sf_magic());




        for(int j=0;j<NUM_FREE_LISTS;j++){
            sf_free_list_heads[j].body.links.next = &sf_free_list_heads[j];
            sf_free_list_heads[j].body.links.prev = &sf_free_list_heads[j];
        }


        sf_free_list_heads[7].body.links.next = remainder;
        sf_free_list_heads[7].body.links.prev = remainder;
        (*(sf_block*)remainder).body.links.next = &sf_free_list_heads[7];
        (*(sf_block*)remainder).body.links.prev = &sf_free_list_heads[7];
    }

    /*if size equals 0, return NULL*/
    if(size==0) return NULL;

    /*adjust block size to include overhead and alignment reqs*/
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);

    i=find_freelist(asize);

    free_blkp = firstFit_blkp(asize,i);
    if(free_blkp==NULL)
        return NULL;
    if(asize+M <= get_size(free_blkp)){
        return split(free_blkp,asize);
    }
    else{
        asize = get_size(free_blkp);
        return place(free_blkp,asize);
    }

}

void sf_free(void *pp) {
    void* bp = (void*)((char*)pp-16);
    size_t size = get_size(bp);
    void* next_blkp = get_next_blkp(bp);
    void* prev_blkp = get_prev_blkp(bp);

    if(!validate_argument(pp))
        abort();

    put(get_hdp(bp),pack(size,0,get_alloc(prev_blkp)));
    put(get_ftp(bp),pack(size,0,get_alloc(prev_blkp))^sf_magic());
    if(next_blkp!=(void*)((char*)sf_mem_end()-16)){
        put(get_hdp(next_blkp),pack(get_size(next_blkp),get_alloc(next_blkp),0));
        put(get_ftp(next_blkp),pack(get_size(next_blkp),get_alloc(next_blkp),0)^sf_magic());
    }
    else{
        put((void*)((char*)sf_mem_end()-8),pack(0,1,0));
    }

    coalesce(bp);




}

void *sf_realloc(void *pp, size_t rsize) {
    if(!validate_argument(pp))
        abort();

    if(rsize==0){
        sf_free(pp);
        return NULL;
    }

    void* bp = (char*)pp-16;
    size_t size = get_size(bp)-16;

    if(size==rsize)
        return pp;
    else if(size<rsize){
        void* new_pp = sf_malloc(rsize);
        if(new_pp == NULL) return NULL;
        memcpy(new_pp,pp,rsize);
        sf_free(pp);
        pp = new_pp;
        return pp;

    }
    else{
        size_t asize;

        if(rsize <= DSIZE)
            asize = 2*DSIZE;
        else
            asize = DSIZE*((rsize+(DSIZE)+(DSIZE-1))/DSIZE);

        if(asize+M <= get_size(bp)){
            return split(bp,asize);
        }
        else{
            asize = get_size(bp);
            return place(bp,asize);
        }
    }

}




















size_t pack(size_t size,size_t alloc,size_t prev_alloc){
    return (size|alloc<<1|prev_alloc);
}

size_t get(void* bp){
    return *((size_t*)(((char*)bp)+8));
}

void put(size_t* p, size_t val){
    *p = val;

}

size_t get_size(void* bp){
    return (get(bp)&BLOCK_SIZE_MASK);
}

size_t get_alloc(void* bp){
    return ((get(bp)&THIS_BLOCK_ALLOCATED)>>1);
}

size_t get_prev_alloc(void* bp){
    return (get(bp)&PREV_BLOCK_ALLOCATED);
}

size_t* get_hdp(void* bp){
    return ((size_t*)(((char*)bp)+8));
}

size_t* get_ftp(void* bp){
    return ((size_t*)(((char*)bp)+get_size(bp)));
}

void* get_prev_blkp(void* bp){
    return ((char*)bp-((((*((size_t*)bp)))^sf_magic())&BLOCK_SIZE_MASK));
}
void* get_next_blkp(void* bp){
    return (char*)bp+get_size(bp);
}

void* coalesce(void *bp){
    //size_t alloc = get_alloc(bp);
    size_t pre_alloc = get_prev_alloc(bp);
    size_t next_alloc = get_alloc(get_next_blkp(bp));
    size_t size = get_size(bp);

    if(pre_alloc&&next_alloc){
        insert_free_blk(bp);
        return bp;
    }

    else if(pre_alloc&&!next_alloc){
        void* nextbp = get_next_blkp(bp);
        size += get_size(nextbp);

        /*reorder the freelist*/
        void* prev_freep = (*(sf_block*)nextbp).body.links.prev;
        void* next_freep = (*(sf_block*)nextbp).body.links.next;
        (*(sf_block*)prev_freep).body.links.next = next_freep;
        (*(sf_block*)next_freep).body.links.prev = prev_freep;
        (*(sf_block*)nextbp).body.links.next = NULL;
        (*(sf_block*)nextbp).body.links.prev = NULL;

        put(get_hdp(bp),pack(size,0,1));
        put(get_ftp(bp),pack(size,0,1)^sf_magic());

        insert_free_blk(bp);
    }

    else if(!pre_alloc&&next_alloc){
        void* prevbp = get_prev_blkp(bp);
        size += get_size(prevbp);

        /*reorder the freelist*/
        void* prev_freep = (*(sf_block*)prevbp).body.links.prev;
        void* next_freep = (*(sf_block*)prevbp).body.links.next;
        (*(sf_block*)prev_freep).body.links.next = next_freep;
        (*(sf_block*)next_freep).body.links.prev = prev_freep;


        put(get_hdp(prevbp),pack(size,0,get_prev_alloc(prevbp)));
        put(get_ftp(prevbp),pack(size,0,get_prev_alloc(prevbp))^sf_magic());

        bp=prevbp;
        insert_free_blk(bp);
    }

    else{
        void* prevbp = get_prev_blkp(bp);
        size += get_size(prevbp);

        /*reorder the freelist*/
        void* prevp_prev_freep = (*(sf_block*)prevbp).body.links.prev;
        void* prevp_next_freep = (*(sf_block*)prevbp).body.links.next;
        (*(sf_block*)prevp_prev_freep).body.links.next = prevp_next_freep;
        (*(sf_block*)prevp_next_freep).body.links.prev = prevp_prev_freep;

        void* nextbp = get_next_blkp(bp);
        size += get_size(nextbp);

        /*reorder the freelist*/
        void* nextp_prev_freep = (*(sf_block*)nextbp).body.links.prev;
        void* nextp_next_freep = (*(sf_block*)nextbp).body.links.next;
        (*(sf_block*)nextp_prev_freep).body.links.next = nextp_next_freep;
        (*(sf_block*)nextp_next_freep).body.links.prev = nextp_prev_freep;
        (*(sf_block*)nextbp).body.links.next = NULL;
        (*(sf_block*)nextbp).body.links.prev = NULL;

        put(get_hdp(prevbp),pack(size,0,get_prev_alloc(prevbp)));
        put(get_ftp(prevbp),pack(size,0,get_prev_alloc(prevbp))^sf_magic());

        bp=prevbp;
        insert_free_blk(bp);
    }

    void* next_blk = get_next_blkp(bp);
    if(next_blk!=(void*)((char*)sf_mem_end()-16)){
        put(get_hdp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),0));
        put(get_ftp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),0)^sf_magic());
    }
    else{
        put((void*)((char*)sf_mem_end()-8),pack(0,1,0));
    }

    return bp;
}


void * firstFit_blkp(size_t asize, int index)
{
    int i;

    while(1){

    for(i = index; i < NUM_FREE_LISTS; i++) {
    sf_block *bp = sf_free_list_heads[i].body.links.next;
    while(bp != &sf_free_list_heads[i]) {
        if(get_size(bp)>=asize){
            return bp;
        }
        bp = bp->body.links.next;
    }
    }
    void *extended_pp = sf_mem_grow();
    if(extended_pp == NULL){
        return NULL;
    }
    else{
        extend_empty_block(extended_pp);
    }



    }
}

void* split(void *free_blkp, size_t asize){
    void* upper_blkp;
    void* lower_blkp;
    lower_blkp = free_blkp;


    /*size information for each block*/
    size_t total_size, upper_size, lower_size=asize;
    total_size = get_size(free_blkp);
    upper_size = total_size-lower_size;

    /*information for upper block*/
    size_t upper_alloc = 0;
    size_t upper_prev_alloc = 1;

    /*reorder the current freelist*/
    /*sf_block * prev_blk = (*(sf_block*)free_blkp).body.links.prev;
    sf_block * next_blk = (*(sf_block*)free_blkp).body.links.next;
    (*prev_blk).body.links.next = next_blk;
    (*next_blk).body.links.prev = prev_blk;*/

    /*set upper field*/
    upper_blkp = (void*)((char*)lower_blkp+lower_size);
    put(get_hdp(upper_blkp), pack(upper_size,upper_alloc,upper_prev_alloc));
    put(get_ftp(upper_blkp), pack(upper_size,upper_alloc,upper_prev_alloc)^sf_magic());
    void* next_blk = get_next_blkp(upper_blkp);
    if(next_blk!=(void*)((char*)sf_mem_end()-16)){
        put(get_hdp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),upper_prev_alloc));
        put(get_ftp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),upper_prev_alloc)^sf_magic());
    }
    else{
        put((void*)((char*)sf_mem_end()-8),pack(0,1,upper_alloc));
    }

    if(get_alloc(free_blkp)==0){
    //(*upper_blkp).body.links.prev = NULL;
    //(*upper_blkp).body.links.next = NULL;

    /*insert and place*/

        insert_free_blk(upper_blkp);
    }
    else{
        coalesce(upper_blkp);
    }
    return place(lower_blkp,lower_size);;
}

void insert_free_blk(void *free_blkp){
    int i;
    size_t size = get_size(free_blkp);

    if(size==32)
        i = 0;
    else if(size>128*M)
        i = 8;
    else
        for(i=1;;i++){
            if( (size>(1<<(i-1))*M) && (size<=(1<<(i))*M))
                break;
        }

    sf_block *bp = sf_free_list_heads[i].body.links.next;
    sf_free_list_heads[i].body.links.next = free_blkp;
    (*bp).body.links.prev = free_blkp;
    (*(sf_block*)free_blkp).body.links.prev = &sf_free_list_heads[i];
    (*(sf_block*)free_blkp).body.links.next = bp;

}

void* place(void *alloc_blkp, size_t size){
    if(get_alloc(alloc_blkp)==0){

        put(get_hdp(alloc_blkp), pack(size,1,get_prev_alloc(get_prev_blkp(alloc_blkp))));
        put(get_ftp(alloc_blkp), pack(size,1,get_prev_alloc(get_prev_blkp(alloc_blkp)))^sf_magic());
        sf_block* prevfbp = (*(sf_block*)alloc_blkp).body.links.prev;
        sf_block* nextfbp = (*(sf_block*)alloc_blkp).body.links.next;
        (*(sf_block*)prevfbp).body.links.next = nextfbp;
        (*(sf_block*)nextfbp).body.links.prev = prevfbp;
    }
    else{
        put(get_hdp(alloc_blkp), pack(size,1,get_prev_alloc(get_prev_blkp(alloc_blkp))));
        put(get_ftp(alloc_blkp), pack(size,1,get_prev_alloc(get_prev_blkp(alloc_blkp)))^sf_magic());

    }

    (*(sf_block*)alloc_blkp).body.links.prev = NULL;
    (*(sf_block*)alloc_blkp).body.links.next = NULL;

    if(if_heap_full()){
        void* epilogue_blkp = (void*)((char*)sf_mem_end()-8);
        put(epilogue_blkp, pack(0,1,1));
    }

    void* next_blk = get_next_blkp(alloc_blkp);
    if(next_blk!=(void*)((char*)sf_mem_end()-16)){
        put(get_hdp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),1));
        put(get_ftp(next_blk),pack(get_size(next_blk),get_alloc(next_blk),1)^sf_magic());
    }


    return (void*)(&((*((sf_block*)alloc_blkp)).body.payload));
}

void extend_empty_block(void* extended_pp){
    void* extended_blkp = (char*)extended_pp - 16;
    put(get_hdp(extended_blkp),pack(PSIZE,0,get_prev_alloc(extended_blkp)));
    put(get_ftp(extended_blkp),pack(PSIZE,0,get_prev_alloc(extended_blkp))^sf_magic());
    void* epilogue_blkp = (void*)((char*)sf_mem_end()-8);
    put(epilogue_blkp, pack(0,1,0));
    coalesce(extended_blkp);
}

int if_heap_full(){
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++) {
    sf_block *bp = sf_free_list_heads[i].body.links.next;
    if(bp != &sf_free_list_heads[i]) {
        return 0;
    }
    }

    return 1;}

int validate_argument(void* pp){
    if(pp==NULL){
        return 0;
    }
    void* bp = (void*)((char*)pp-16);
    void* prologue_blkp = sf_mem_start();
    void* epilogue_blkp = (void*)((char*)sf_mem_end()-8);
    void* prev_blkp = get_prev_blkp(bp);
    size_t size = get_size(bp);


    if(get_alloc(bp)==0)
        return 0;
    if((void*)get_hdp(bp)<=(void*)((char*)prologue_blkp+32))
        return 0;
    if((void*)((char*)bp+size)>=epilogue_blkp)
        return 0;
    if(size<32)
        return 0;
    if(get_prev_alloc(bp)==0&&get_alloc(prev_blkp)!=0)
        return 0;
    if(((*((size_t*)get_ftp(bp)))^sf_magic()) != get(bp))
        return 0;
    return 1;
}

size_t find_freelist(size_t asize){
    int i;

    if(asize==32)
        i = 0;
    else if(asize>128*M)
        i = 8;
    else
        for(i=1;;i++){
            if( (asize>(1<<(i-1))*M) && (asize<=(1<<(i))*M))
                break;
        }
    return i;
}