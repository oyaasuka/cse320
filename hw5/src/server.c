#include "server.h"
#include "debug.h"
#include "csapp.h"
#include "protocol.h"
#include "trader.h"

void convertToHostBytes(BRS_PACKET_HEADER *hdr);
void convertToNetBytes(BRS_PACKET_HEADER *hdr);
BRS_PACKET_HEADER * create_sed_hdr(BRS_PACKET_TYPE t,uint16_t s);
char* brs_login(TRADER* user, int connfd, BRS_PACKET_HEADER *rev_hdr,void **rev_payloadp);


void *brs_client_service(void *arg){
    int connfd = *(int*)arg;
    BRS_PACKET_HEADER *rev_hdr;
    void **rev_payloadp;
    TRADER *user=NULL;
    BRS_STATUS_INFO * status = Malloc(sizeof(BRS_STATUS_INFO));

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
        else{
            convertToHostBytes(rev_hdr);
        }
        if(rev_hdr->type==BRS_LOGIN_PKT && user ==NULL){
            debug("[%d] LOGIN package recieved", connfd);
            char* name;

            if((name=brs_login(user,connfd,rev_hdr,rev_payloadp))==NULL) return NULL;

            //make pkg
            BRS_PACKET_HEADER* sed_hdr;
            if((sed_hdr=create_sed_hdr(BRS_ACK_PKT,0)) ==NULL){
                free(sed_hdr);
                free(name);
                return NULL;
            }
             //TRADER *trader = Malloc(sizeof( TRADER));
            user= trader_login(connfd, name);
            trader_send_packet(user, sed_hdr,NULL);
            free(sed_hdr);
            free(name);
        }
        else if(rev_hdr->type==BRS_LOGIN_PKT && user !=NULL){
            debug("Already logined");
            continue;
        }
        else if(rev_hdr->type!=BRS_LOGIN_PKT && user ==NULL){
            debug("Login requires");
            return NULL;
        }
        else{
            if(rev_hdr->type==BRS_STATUS_PKT){
                debug("[%d] STATUS package recieved", connfd);

                //make sending package
                BRS_PACKET_HEADER* sed_hdr;
                if((sed_hdr=create_sed_hdr(BRS_ACK_PKT,sizeof(BRS_STATUS_INFO))) ==NULL){
                    free(sed_hdr);
                    continue;
                }
                //get status
                exchange_get_status(exchange,status);
                trader_send_packet(user, sed_hdr, status);
                free(sed_hdr);

            }
            else if(rev_hdr->type==BRS_DEPOSIT_PKT){
                debug("[%d] DEPOSIT package recieved", connfd);

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size*sizeof(BRS_FUNDS_INFO));
                for(int i=0;i<size;i++){
                    memcpy(p,*rev_payloadp,size);
                }
                int amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                trader_increase_balance(user, amount);

                //make sending package
                BRS_PACKET_HEADER* sed_hdr;
                if((sed_hdr=create_sed_hdr(BRS_ACK_PKT,sizeof(BRS_STATUS_INFO))) ==NULL){
                    free(sed_hdr);
                    continue;
                }
                //get status
                exchange_get_status(exchange,status);
                status->balance=htonl(ntohl(status->balance)+amount);

                trader_send_packet(user, sed_hdr, status);
                free(sed_hdr);
            }
            else if(rev_hdr->type==BRS_WITHDRAW_PKT){
                debug("[%d] WITHDRAW package recieved", connfd);

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size*sizeof(BRS_FUNDS_INFO));
                for(int i=0;i<size;i++){
                    memcpy(p,*rev_payloadp,size);
                }
                int amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);

                int pkg_type;
                int pkg_size;
                if(amount<=ntohl(status->balance)){
                    pkg_type = BRS_ACK_PKT;
                    pkg_size = sizeof(BRS_STATUS_INFO);
                    status->balance = htonl(ntohl(status->balance)-amount);
                    trader_decrease_balance(user,amount);
                }
                else{
                    pkg_type = BRS_NACK_PKT;
                    pkg_size = 0;
                }

                //make sending package
                BRS_PACKET_HEADER* sed_hdr;
                if((sed_hdr=create_sed_hdr(pkg_type,pkg_size)) ==NULL){
                    free(sed_hdr);
                    continue;
                }
                //get status
                if(pkg_type==BRS_ACK_PKT){
                    exchange_get_status(exchange,status);
                    //status->balance=htonl(amount);

                    trader_send_packet(user, sed_hdr, status);
                }
                else{
                    trader_send_packet(user, sed_hdr, NULL);
                }
                free(sed_hdr);
            }
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

BRS_PACKET_HEADER * create_sed_hdr(BRS_PACKET_TYPE t,uint16_t s){
    struct timespec current;
    if(clock_gettime(CLOCK_MONOTONIC, &current)!=0){
            return NULL;
    }
    BRS_PACKET_HEADER *sed_hdr = Malloc(sizeof(BRS_PACKET_HEADER));
    sed_hdr->type = t;
    sed_hdr->size = s;
    sed_hdr->timestamp_sec = current.tv_sec;
    sed_hdr->timestamp_nsec = current.tv_nsec;
    convertToNetBytes(sed_hdr);
    return sed_hdr;
}


char* brs_login(TRADER* user, int connfd, BRS_PACKET_HEADER *rev_hdr,void **rev_payloadp){

    //get the payload
    int size = rev_hdr->size;
    char *name = Malloc(size*sizeof(char));
    for(int i=0;i<size;i++){
        if(memcpy(name,*rev_payloadp,size)<0) return NULL;
    }
    free(rev_hdr);
    free(rev_payloadp);
    //make sending package
    debug("[%d] LOGIN '%s'",connfd,name);
    return name;
}