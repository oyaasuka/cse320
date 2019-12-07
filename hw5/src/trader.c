#include "debug.h"
#include "csapp.h"
#include "string.h"
#include "help.h"


TRADER* find_trader_in_map(MAP* map, int fd, char*name);
BRS_PACKET_HEADER * create_sed_hdr(BRS_PACKET_TYPE t,uint16_t s);
void convertToNetBytes(BRS_PACKET_HEADER *hdr);


extern TRADER user;
static MAP *map;
//static orderid_t order = 0;

int trader_init(){
    debug("initializing trader module");
    if((map = Malloc(sizeof(MAP)))<0) return -1;
    map->header = NULL;
    map->size = 0;
    pthread_mutex_init(&map->mutex,NULL);
    return 0;
}

void trader_fini(){
    debug("finalizing trader module");
    TRADER* header = map->header;
    while(header!=NULL){
        free(header->name);
        free(header->status);
        free(header);
        header= header->next;
    }
    free(map);
}

TRADER *trader_login(int fd, char *name){
    TRADER *trader;
    int size= sizeof(name);
    char* n = Malloc(size);
    memcpy(n, name, size);

    if((trader=find_trader_in_map(map,fd,name))==NULL){
        trader = Malloc(sizeof(TRADER));
        trader->fd = fd;
        trader->name = n;
        trader->ref = 0;
        trader->inventory = 0;
        trader->balance = 0;
        trader->next = NULL;
        trader->status = Malloc(sizeof(BRS_STATUS_INFO));
        (*trader).status->balance = 0;               // Trader's account balance
        (*trader).status->inventory = 0;          // Trader's inventory
        (*trader).status->bid = 0;                   // Current highest bid price
        (*trader).status->ask = 0;                   // Current lowest ask price
        (*trader).status->last = 0;                  // Last trade price
        (*trader).status->orderid = 0;             // Order ID (for BUY, SELL, CANCEL)
        (*trader).status->quantity = 0;
        pthread_mutexattr_init(&trader->attr);
        pthread_mutexattr_settype(&trader->attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&trader->mutex,&trader->attr);

        pthread_mutex_lock(&trader->mutex);
        pthread_mutex_lock(&map->mutex);
        if(map->header==NULL) {
            map->header = trader;
        }
        else{
            TRADER* header = map->header;
            while(header->next!=NULL){
                header = header->next;
            }
            header->next = trader;
        }
        pthread_mutex_unlock(&map->mutex);

        debug("Create new trader");
        trader_ref(trader, "for newly created trader");
    }
    else{
        pthread_mutex_lock(&trader->mutex);
        debug("Login existing trader");
    }

    trader->logged = 1;
    pthread_mutex_unlock(&trader->mutex);
    return trader_ref(trader, "because trader has logged in");
}

void trader_logout(TRADER *trader){

    pthread_mutex_lock(&trader->mutex);

    debug("Log out trader [%s]",trader->name);
    trader->logged = 0;
    trader_unref(trader, "because trader has logged out");

    pthread_mutex_unlock(&trader->mutex);

}


TRADER *trader_ref(TRADER *trader, char *why){

    pthread_mutex_lock(&trader->mutex);

    trader->ref = trader->ref+1;
    debug("Increase ref on trader [%s] (%d->%d) %s",trader->name,trader->ref-1,trader->ref, why);

    pthread_mutex_unlock(&trader->mutex);

    return trader;
}

void trader_unref(TRADER *trader, char *why){

    pthread_mutex_lock(&trader->mutex);

    trader->ref = trader->ref-1;
    debug("Decrease ref on trader [%s] (%d->%d) %s",trader->name,trader->ref,trader->ref-1, why);

    pthread_mutex_unlock(&trader->mutex);
}


int trader_send_packet(TRADER *trader, BRS_PACKET_HEADER *pkt, void *data){
    pthread_mutex_lock(&trader->mutex);

    int type = pkt->type;
    int fd = trader->fd;


    if(type==BRS_ACK_PKT){
        debug("Send packet (clientfd=%d, type=ACK) for trader [%s]", trader->fd, trader->name);
        if(proto_send_packet(fd, pkt, data)<0) return -1;
    }
    else{
        debug("Send packet (clientfd=%d, type=NACK) for trader [%s]", trader->fd, trader->name);
        if(proto_send_packet(fd, pkt, NULL)<0) return -1;
    }
    free(pkt);
    free(data);
    pthread_mutex_unlock(&trader->mutex);

    return 0;
}


int trader_broadcast_packet(BRS_PACKET_HEADER *pkt, void *data){//how to share data between files?
    debug("here 3");
    pthread_mutex_lock(&map->mutex);

    TRADER *header = map->header;
    while(header!=NULL){
        if(header->logged==1){
            if(trader_send_packet(header,pkt,data)<0)
                debug("trader_broadcast_packet: wrong");
        }
        header = header->next;
    }


    pthread_mutex_unlock(&map->mutex);
    return 0;
}


int trader_send_ack(TRADER *trader, BRS_STATUS_INFO *info){

    BRS_PACKET_HEADER *pkt = create_sed_hdr(BRS_ACK_PKT,sizeof(BRS_STATUS_INFO));

    if(trader_send_packet(trader, pkt, info)<0) {
        return -1;
    }

    return 0;
}


int trader_send_nack(TRADER *trader){

    BRS_PACKET_HEADER *pkt = create_sed_hdr(BRS_NACK_PKT,0);

    if(trader_send_packet(trader, pkt, NULL)<0) {
        return -1;
    }

    return 0;
}


void trader_increase_balance(TRADER *trader, funds_t amount){

    pthread_mutex_lock(&trader->mutex);
    trader->balance = trader->balance + amount;
    (*trader).status->balance = trader->balance;
    debug("increase balance: %d->%d",trader->balance-amount,trader->balance);

    pthread_mutex_unlock(&trader->mutex);
}


int trader_decrease_balance(TRADER *trader, funds_t amount){

    pthread_mutex_lock(&trader->mutex);

    funds_t balance = trader->balance;
    if(balance>=amount){
        trader->balance = balance - amount;
        (*trader).status->balance = trader->balance;
        debug("decrease balance: %d->%d",balance,trader->balance);

        pthread_mutex_unlock(&trader->mutex);
        return amount;
    }
    else{

        pthread_mutex_unlock(&trader->mutex);
        return -1;
    }

}


void trader_increase_inventory(TRADER *trader, quantity_t quantity){

    pthread_mutex_lock(&trader->mutex);

    trader->inventory = trader->inventory + quantity;
    (*trader).status->inventory = trader->inventory;
    debug("increase quantity: %d->%d",trader->inventory-quantity,trader->inventory);

    pthread_mutex_unlock(&trader->mutex);
}

int trader_decrease_inventory(TRADER *trader, quantity_t quantity){

    pthread_mutex_lock(&trader->mutex);

    quantity_t inventory = trader->inventory;
    if(inventory>=quantity){
        trader->inventory = inventory - quantity;
        (*trader).status->inventory = trader->inventory;
        debug("decrease quantity: %d->%d",inventory,trader->inventory);

        pthread_mutex_unlock(&trader->mutex);
        return quantity;
    }
    else{

        pthread_mutex_unlock(&trader->mutex);
        return -1;
    }
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


void convertToNetBytes(BRS_PACKET_HEADER *hdr){
    hdr->size = htons(hdr->size);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

}

TRADER* find_trader_in_map(MAP* map, int fd, char*name){

    pthread_mutex_lock(&map->mutex);

    TRADER* header = map->header;
    while(header!=NULL){
        //debug("%s %s",header->name,name);
        if(strcmp(header->name,name)==0){
            header->fd = fd;
            pthread_mutex_unlock(&map->mutex);
            return header;
        }
        header = header->next;
    }

    pthread_mutex_unlock(&map->mutex);
    return NULL;
}
