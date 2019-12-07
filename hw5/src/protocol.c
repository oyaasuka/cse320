#include <errno.h>

#include "protocol.h"
#include "debug.h"
#include "csapp.h"

int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload){
    uint16_t payload_size = ntohs(hdr->size);

    Rio_writen(fd, hdr, sizeof(BRS_PACKET_HEADER));
    if(payload!=NULL && payload_size>0){
        Rio_writen(fd, payload, payload_size);
    }
    return 0;
}

int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp){
    int payload_size;
    if((Rio_readn(fd, hdr, sizeof(BRS_PACKET_HEADER))) < sizeof(BRS_PACKET_HEADER)){
        debug("EOF on fd: %d",fd);
        errno = ENOENT;
        return -1;
    }

    payload_size = ntohs(hdr->size);
    if(payloadp!=NULL && payload_size > 0){

        *payloadp = Malloc(payload_size*sizeof(char));//!!!!!!
        if(Rio_readn(fd,*payloadp,payload_size)<payload_size){
            debug("aEOF on fd: %d",fd);
            errno = ENOENT;
            return -1;
        }
    }
    return 0;
}

