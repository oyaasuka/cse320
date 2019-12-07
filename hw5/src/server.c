#include "server.h"
#include "debug.h"
#include "csapp.h"
#include "protocol.h"
#include "help.h"


void convertToHostBytes(BRS_PACKET_HEADER *hdr);
TRADER* brs_login(int connfd, char*name);
int brs_buy(TRADER* user, quantity_t quantity, funds_t price);
int brs_sell(TRADER *user, quantity_t quantity, funds_t price);
void convertPayloadToNetBytes(BRS_STATUS_INFO *P);

static orderid_t order = 0;

void *brs_client_service(void *arg){
    int connfd = *(int*)arg;
    BRS_PACKET_HEADER *rev_hdr;
    void **rev_payloadp;
    TRADER *user;

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
            debug("[%d] Logging out trader",connfd);
            trader_logout(user);
            debug("[%d] End client service",connfd);
            creg_unregister(client_registry, connfd);

            return NULL;
        }
        else{
            convertToHostBytes(rev_hdr);
        }
        if(rev_hdr->type==BRS_LOGIN_PKT && user ==NULL){


            char *name = Malloc(rev_hdr->size);

            if(memcpy(name,*rev_payloadp,rev_hdr->size)<0) return NULL;
            free(rev_hdr);
            free(rev_payloadp);
            debug("[%d] LOGIN package recieved", connfd);

            //LOGIN
            user= brs_login(connfd, name);
            if(user == NULL){
                return NULL;//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            }

            struct timespec current;
            if(clock_gettime(CLOCK_MONOTONIC, &current)!=0){
                return NULL;
            }
            BRS_PACKET_HEADER *hdr = Malloc(sizeof(BRS_PACKET_HEADER));
            hdr->type = BRS_ACK_PKT;
            hdr->size = htons(0);
            hdr->timestamp_sec = htonl(current.tv_sec);
            hdr->timestamp_nsec = htonl(current.tv_nsec);


            proto_send_packet(connfd, hdr, NULL);
            free(hdr);
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

                pthread_mutex_lock(&user->mutex);

                BRS_STATUS_INFO * status = user->status;
                exchange_get_status(exchange,status);

                BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                memcpy(data, status,sizeof(BRS_STATUS_INFO));

                pthread_mutex_unlock(&user->mutex);
                convertPayloadToNetBytes(data);

                trader_send_ack(user,data);
            }
            else if(rev_hdr->type==BRS_DEPOSIT_PKT){

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size);

                memcpy(p,*rev_payloadp,size);

                funds_t amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] DEPOSIT package recieved", connfd);

                trader_increase_balance(user, amount);


                pthread_mutex_lock(&user->mutex);
                BRS_STATUS_INFO * status = user->status;
                exchange_get_status(exchange,status);

                BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                memcpy(data, status,sizeof(BRS_STATUS_INFO));
                pthread_mutex_unlock(&user->mutex);
                convertPayloadToNetBytes(data);


                trader_send_ack(user,data);


            }
            else if(rev_hdr->type==BRS_WITHDRAW_PKT){

                //get the payload
                int size = rev_hdr->size;
                BRS_FUNDS_INFO *p = Malloc(size);

                memcpy(p,*rev_payloadp,size);

                funds_t amount = ntohl(p->amount);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] WITHDRAW package recieved", connfd);

                pthread_mutex_lock(&user->mutex);
                BRS_STATUS_INFO * status = user->status;

                if(amount<=status->balance){
                    trader_decrease_balance(user,amount);
                    exchange_get_status(exchange,status);

                    BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                    memcpy(data, status,sizeof(BRS_STATUS_INFO));
                    pthread_mutex_unlock(&user->mutex);

                    convertPayloadToNetBytes(data);
                    trader_send_ack(user,data);
                }
                else{
                    pthread_mutex_unlock(&user->mutex);
                    trader_send_nack(user);
                }


            }
            else if(rev_hdr->type==BRS_ESCROW_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ESCROW_INFO *p = Malloc(size);

                memcpy(p,*rev_payloadp,size);

                quantity_t quantity = ntohl(p->quantity);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] ESCROW package recieved", connfd);

                trader_increase_inventory(user, quantity);

                pthread_mutex_lock(&user->mutex);
                BRS_STATUS_INFO * status = user->status;
                exchange_get_status(exchange,status);

                BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                memcpy(data, status,sizeof(BRS_STATUS_INFO));
                pthread_mutex_unlock(&user->mutex);
                convertPayloadToNetBytes(data);

                trader_send_ack(user,data);
            }
            else if(rev_hdr->type==BRS_RELEASE_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ESCROW_INFO *p = Malloc(size);

                memcpy(p,*rev_payloadp,size);

                quantity_t quantity = ntohl(p->quantity);
                free(p);
                free(rev_hdr);
                free(rev_payloadp);

                debug("[%d] RELEASE package recieved", connfd);

                pthread_mutex_lock(&user->mutex);
                BRS_STATUS_INFO * status = user->status;

                if(quantity<=status->inventory){
                    trader_decrease_inventory(user,quantity);
                    exchange_get_status(exchange,status);

                    BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                    memcpy(data, status,sizeof(BRS_STATUS_INFO));
                    pthread_mutex_unlock(&user->mutex);

                    convertPayloadToNetBytes(data);
                    trader_send_ack(user,data);
                }
                else{
                    pthread_mutex_unlock(&user->mutex);
                    trader_send_nack(user);
                }
            }
            else if(rev_hdr->type==BRS_BUY_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ORDER_INFO* pkg = Malloc(size);
                memcpy(pkg,*(BRS_ORDER_INFO**)rev_payloadp,rev_hdr->size);
                pkg->quantity = ntohl(pkg->quantity);
                pkg->price = ntohl(pkg->price);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] BUY package recieved", connfd);

                quantity_t quantity =pkg->quantity;
                funds_t price = pkg->price;

                pthread_mutex_lock(&user->mutex);

                BRS_STATUS_INFO * status = user->status;
                if(quantity*price<=status->balance){
                    brs_buy(user, quantity, price);
                    exchange_get_status(exchange,status);
                    BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                    memcpy(data, status,sizeof(BRS_STATUS_INFO));
                    pthread_mutex_unlock(&user->mutex);
                    data->orderid = ++order;
                    convertPayloadToNetBytes(data);
                    trader_send_ack(user,data);
                }
                else{
                    pthread_mutex_unlock(&user->mutex);
                    trader_send_nack(user);
                }
            }
            else if(rev_hdr->type==BRS_SELL_PKT){
                //get the payload
                int size = rev_hdr->size;
                BRS_ORDER_INFO* pkg = Malloc(size);
                memcpy(pkg,*(BRS_ORDER_INFO**)rev_payloadp,rev_hdr->size);
                pkg->quantity = ntohl(pkg->quantity);
                pkg->price = ntohl(pkg->price);
                free(rev_hdr);
                free(rev_payloadp);
                debug("[%d] SELL package recieved", connfd);

                quantity_t quantity =pkg->quantity;
                funds_t price = pkg->price;

                pthread_mutex_lock(&user->mutex);

                BRS_STATUS_INFO * status = user->status;
                if(quantity*price<=status->balance){
                    brs_sell(user, quantity, price);
                    exchange_get_status(exchange,status);
                    BRS_STATUS_INFO * data = Malloc(sizeof(BRS_STATUS_INFO));
                    memcpy(data, status,sizeof(BRS_STATUS_INFO));
                    pthread_mutex_unlock(&user->mutex);
                    data->orderid = ++order;
                    convertPayloadToNetBytes(data);
                    trader_send_ack(user,data);
                }
                else{
                    pthread_mutex_unlock(&user->mutex);
                    trader_send_nack(user);
                }



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

TRADER* brs_login(int connfd, char*name){
    debug("[%d] LOGIN '%s'",connfd,name);
    TRADER* user = trader_login(connfd, name);
    return user;
}


int brs_buy(TRADER *user, quantity_t quantity, funds_t price){

    debug("brs_buy: quantity:%d, limit: %d",quantity,price);
    if(exchange_post_buy(exchange,user,quantity,price)>0){
        debug("here1");
        return 0;
    }
    return -1;
}

int brs_sell(TRADER *user, quantity_t quantity, funds_t price){
    debug("brs_sell: quantity:%d, limit: %d",quantity,price);

    if(exchange_post_sell(exchange,user,quantity,price)>0){
        debug("here2");
        return 0;
    }
    return -1;

}

void convertPayloadToNetBytes(BRS_STATUS_INFO *P){
    (*P).balance = htonl((*P).balance);               // Trader's account balance
    (*P).inventory = htonl((*P).inventory);          // Trader's inventory
    (*P).bid = htonl((*P).bid);                   // Current highest bid price
    (*P).ask = htonl((*P).ask);                   // Current lowest ask price
    (*P).last = htonl((*P).last);                  // Last trade price
    (*P).orderid = htonl((*P).orderid);             // Order ID (for BUY, SELL, CANCEL)
    (*P).quantity = htonl((*P).quantity);

}