#include "client_registry.h"
#include "csapp.h"
#include "debug.h"
#include "trader.h"

typedef struct client_registry{
    int *buf;
    int length;
    int size;
    sem_t mutex;
    sem_t empty;//a flag to indicate emptyness
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init(){//?
    int n = FD_SETSIZE-4;
    CLIENT_REGISTRY * cr = Malloc(sizeof(CLIENT_REGISTRY));
    cr->buf = Malloc(MAX_TRADERS);
    cr->length = n;
    cr->size = 0;
    Sem_init(&cr->mutex, 0, 1);
    Sem_init(&cr->empty, 0, 1);

    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr){
    free(cr->buf);
    free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd){
    P(&cr->mutex);
    int i =0;

    while(i<cr->length){
        if(cr->buf[i]==0){
            if(cr->size==0) P(&cr->empty);
            cr->buf[i] = fd;
            cr->size = cr->size+1;
            V(&cr->mutex);
            debug("Register client fd %d (total connected:%d)",fd,cr->size);
            return 0;
        }
        i++;
    }

    V(&cr->mutex);

    return -1;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd){
    P(&cr->mutex);
    int i =0;

    while(i<cr->length){

        if(cr->buf[i]==fd){
            cr->buf[i] = 0;
            cr->size = cr->size-1;
            if(cr->size==0) V(&cr->empty);
            V(&cr->mutex);
            debug("Unregister client fd %d (total connected:%d)",fd,cr->size);
            return 0;
        }
        i++;
    }

    V(&cr->mutex);

    return -1;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    P(&cr->empty);
    if(cr->size==0) debug("Registry empty");
    V(&cr->empty);
    return;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
    int n = cr->length;

    for(int i=0;i<n;i++){
        if(cr->buf[i]!=0){
            int fd = cr->buf[i];
            shutdown(fd,SHUT_RD);
            debug("Shutting down client %d",fd);
        }
    }
}

//need to debug this file