#include "server.h"
#include "debug.h"
#include "csapp.h"
#include "protocol.h"
#include "trader.h"

void convertToHostBytes(BRS_PACKET_HEADER *hdr);
void convertToNetBytes(BRS_PACKET_HEADER *hdr);


void *brs_client_service(void *arg){
    int connfd = *(int*)arg;
    BRS_PACKET_HEADER *rev_hdr;
    void **rev_payloadp;

    free(arg);
    Pthread_detach(pthread_self());
    debug("[%d] Starting client service", connfd);
    creg_register(client_registry, connfd);//??

    while(1){
        rev_hdr = Malloc(sizeof(BRS_PACKET_HEADER));
        rev_payloadp = Malloc(sizeof(void*));
        if(proto_recv_packet(connfd,rev_hdr,rev_payloadp)==-1){//how to understand null?
            free(rev_hdr);
            free(rev_payloadp);
            continue;
        }
        if(rev_hdr->type==BRS_LOGIN_PKT){
            debug("LOGIN package recieved");
            convertToHostBytes(rev_hdr);
            int size = rev_hdr->size;
            //debug("%d\n",size);
            char* name = Malloc(size*sizeof(char));
            for(int i=0;i<size;i++){
                memcpy(name,*rev_payloadp,size);
            }
            free(rev_hdr);
            free(rev_payloadp);

            struct timespec current;
            if(clock_gettime(CLOCK_MONOTONIC, &current)!=0){
                continue;
            }
            BRS_PACKET_HEADER *sed_hdr = Malloc(sizeof(BRS_PACKET_HEADER));
            sed_hdr->type = BRS_ACK_PKT;
            sed_hdr->size = 0;
            sed_hdr->timestamp_sec = current.tv_sec;
            sed_hdr->timestamp_nsec = current.tv_nsec;
            convertToNetBytes(sed_hdr);
            free(sed_hdr);

            //TRADER *trader = Malloc(sizeof( TRADER));
            trader_login(connfd, name);
            proto_send_packet(connfd, sed_hdr, NULL);
        }

    }
}

void convertToHostBytes(BRS_PACKET_HEADER *hdr){
    hdr->size = ntohs(hdr->size);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

}

void convertToNetBytes(BRS_PACKET_HEADER *hdr){
    hdr->size = htons(hdr->size);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

}