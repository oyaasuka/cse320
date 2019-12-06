#include "server.h"
#include "debug.h"
#include "csapp.h"
#include "protocol.h"
#include "trader.h"

void convertToHostBytes(BRS_PACKET_HEADER *hdr);
void convertToNetBytes(BRS_PACKET_HEADER *hdr);
BRS_PACKET_HEADER * create_sed_hdr(BRS_PACKET_TYPE t,uint16_t s);
TRADER* brs_login(int connfd, char*name);
int brs_buy(TRADER* user, quantity_t quantity, funds_t price);
int brs_sell(TRADER *user, quantity_t quantity, funds_t price);
static orderid_t order = 0;

void *brs_client_service(void *arg){
    int connfd = *(int*)arg;
    BRS_PACKET_HEADER *rev_hdr;
    void **rev_payloadp;
    TRADER *user;
    BRS_STATUS_INFO * status = Malloc(sizeof(BRS_STATUS_INFO));

    free(arg);
    Pthread_detach(pthread_self());
    debug("[%d] Starting client service", connfd);

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


            char *name = Malloc(rev_hdr->size);

            if(memcpy(name,*rev_payloadp,rev_hdr->size)<0) return NULL;
            creg_register(client_registry, connfd);//??
            //debug("<= %d.%d: type=LOGIN, size=%d, user: '%s'", rev_hdr->timestamp_sec, rev_hdr->timestamp_nsec, rev_hdr->size, name);
            free(rev_hdr);
            free(rev_payloadp);
            debug("[%d] LOGIN package recieved", connfd);

            //LOGIN
            user= brs_login(connfd, name);
            if(user == NULL){
                return NULL;//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            }

            //make pkg
            BRS_PACKET_HEADER* sed_hdr;
            if((sed_hdr=create_sed_hdr(BRS_ACK_PKT,0)) ==NULL){
                free(sed_hdr);
                free(name);
                return NULL;
            }
            trader_send_packet(user, sed_hdr,NULL);
            //debug("[%d] LOGIN package recieved", connfd);
            //convertToHostBytes(sed_hdr);
            //debug("=> %d.%d: type=ACK, size=%d (no payload)", sed_hdr->timestamp_sec, sed_hdr->timestamp_nsec, sed_hdr->size);
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

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size*sizeof(BRS_FUNDS_INFO));

                memcpy(p,*rev_payloadp,size);

                funds_t amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] DEPOSIT package recieved", connfd);

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

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size*sizeof(BRS_FUNDS_INFO));

                memcpy(p,*rev_payloadp,size);

                funds_t amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] WITHDRAW package recieved", connfd);

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
            else if(rev_hdr->type==BRS_ESCROW_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ESCROW_INFO *p = Malloc(size*sizeof(BRS_ESCROW_INFO));

                memcpy(p,*rev_payloadp,size);

                quantity_t quantity = ntohl(p->quantity);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] ESCROW package recieved", connfd);

                trader_increase_inventory(user, quantity);

                //make sending package
                BRS_PACKET_HEADER* sed_hdr;
                if((sed_hdr=create_sed_hdr(BRS_ACK_PKT,sizeof(BRS_STATUS_INFO))) ==NULL){
                    free(sed_hdr);
                    continue;
                }
                //get status
                exchange_get_status(exchange,status);
                status->inventory=htonl(ntohl(status->inventory)+quantity);

                trader_send_packet(user, sed_hdr, status);
                free(sed_hdr);
            }
            else if(rev_hdr->type==BRS_RELEASE_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ESCROW_INFO *p = Malloc(size*sizeof(BRS_ESCROW_INFO));

                memcpy(p,*rev_payloadp,size);

                quantity_t quantity = ntohl(p->quantity);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);

                debug("[%d] RELEASE package recieved", connfd);

                int pkg_type;
                int pkg_size;
                if(quantity<=ntohl(status->inventory)){
                    pkg_type = BRS_ACK_PKT;
                    pkg_size = sizeof(BRS_STATUS_INFO);
                    status->inventory = htonl(ntohl(status->inventory)-quantity);
                    trader_decrease_inventory(user,quantity);
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
            else if(rev_hdr->type==BRS_BUY_PKT){
                //get the payload
                BRS_ORDER_INFO* pkg = Malloc(sizeof(rev_hdr->size));
                memcpy(pkg,*(BRS_ORDER_INFO**)rev_payloadp,rev_hdr->size);
                pkg->quantity = ntohl(pkg->quantity);
                pkg->price = ntohl(pkg->price);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] BUY package recieved", connfd);

                quantity_t quantity =pkg->quantity;
                funds_t price = pkg->price;

                int pkg_type;
                int pkg_size;
                if(quantity*price<=ntohl(status->balance)){
                    pkg_type = BRS_ACK_PKT;
                    pkg_size = sizeof(BRS_STATUS_INFO);
                    status->balance = htonl(ntohl(status->balance)-quantity*price);
                    status->orderid = htonl(++order);
                    brs_buy(user, quantity, price);
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
            else if(rev_hdr->type==BRS_SELL_PKT){
                //get the payload
                BRS_ORDER_INFO* pkg = Malloc(sizeof(rev_hdr->size));
                memcpy(pkg,*(BRS_ORDER_INFO**)rev_payloadp,rev_hdr->size);
                pkg->quantity = ntohl(pkg->quantity);
                pkg->price = ntohl(pkg->price);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] SELL package recieved", connfd);

                quantity_t quantity =pkg->quantity;
                funds_t price = pkg->price;

                int pkg_type;
                int pkg_size;
                if(quantity<=ntohl(status->inventory)){
                    pkg_type = BRS_ACK_PKT;
                    pkg_size = sizeof(BRS_STATUS_INFO);
                    status->inventory = htonl(ntohl(status->inventory)-quantity);
                    status->orderid = htonl(++order);
                    brs_sell(user, quantity, price);
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
            else if(rev_hdr->type==BRS_CANCEL_PKT){
                debug("hhhhhhh\n\n\n\nhhhh");
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


TRADER* brs_login(int connfd, char*name){
    debug("[%d] LOGIN '%s'",connfd,name);
    TRADER* user = trader_login(connfd, name);
    return user;
}


int brs_buy(TRADER *user, quantity_t quantity, funds_t price){

    debug("brs_buy: quantity:%d, limit: %d",quantity,price);
    //trader_decrease_balance(user,quantity*price);
    if(exchange_post_buy(exchange,user,quantity,price)>0){
        return 0;
    }
    return -1;
}

int brs_sell(TRADER *user, quantity_t quantity, funds_t price){
    debug("brs_sell: quantity:%d, limit: %d",quantity,price);

    if(exchange_post_sell(exchange,user,quantity,price)>0){
        return 0;
    }
    return -1;

}