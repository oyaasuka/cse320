#include "trader.h"
#include "debug.h"
#include "csapp.h"
#include "string.h"

typedef struct trader {
    int fd;
    char* name;
    int ref;
    quantity_t inventory;
    funds_t balance;
    TRADER* next;
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
}TRADER;

typedef struct map
{
    TRADER* header;
    int size;
    pthread_mutex_t mutex;
}MAP;

TRADER* find_trader(MAP* map, int fd, char*name);

static MAP *map;

int trader_init(){
    debug("initializing trader module");
    if((map = Malloc(sizeof(MAP)))<0) return -1;
    map->header = NULL;
    map->size = 0;
    pthread_mutex_init(&map->mutex,NULL);
    return 0;
}

void trader_fini(){
    debug("finalizing trader module")
    TRADER* header = map->header;
    while(header!=NULL){
        free(header);
        header= header->next;
    }
    free(map);
}

TRADER *trader_login(int fd, char *name){
    TRADER *trader;
    if((trader=find_trader(map,fd,name))==NULL){
        trader = Malloc(sizeof(TRADER));
        trader->fd = fd;
        trader->name = name;
        trader->ref = 0;
        trader->inventory = 0;
        trader->balance = 0;
        trader->next = NULL;
        pthread_mutexattr_init(&trader->attr);
        pthread_mutexattr_settype(&trader->attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&trader->mutex,&trader->attr);

        pthread_mutex_lock(&map->mutex);
        if(map->header==NULL) map->header = trader;
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
        debug("Login existing trader");
    }

    return trader_ref(trader, "because trader has logged in");;
}

void trader_logout(TRADER *trader){

    pthread_mutex_lock(&trader->mutex);
    debug("Log out trader [%s]",trader->name);
    pthread_mutex_unlock(&trader->mutex);

    trader_unref(trader, "because trader has logged out");

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



TRADER* find_trader(MAP* map, int fd, char*name){

    pthread_mutex_lock(&map->mutex);
    TRADER* header = map->header;
    while(header!=NULL){
        if(header->fd==fd && header->name == name){
            pthread_mutex_unlock(&map->mutex);

            return header;
        }
    }
    pthread_mutex_unlock(&map->mutex);

    return NULL;
}

/*
 * Send a packet to the client for a trader.
 *
 * @param trader  The TRADER object for the client who should receive
 * the packet.
 * @param pkt  The packet to be sent.
 * @param data  Data payload to be sent, or NULL if none.
 * @return 0 if transmission succeeds, -1 otherwise.
 *
 * Once a client has connected and successfully logged in, this function
 * should be used to send packets to the client, as opposed to the lower-level
 * proto_send_packet() function.  The reason for this is that the present
 * function will obtain exclusive access to the trader before calling
 * proto_send_packet().  The fact that exclusive access is obtained before
 * sending means that multiple threads can safely call this function to send
 * to the client, and these calls will be properly serialized.
 */
int trader_send_packet(TRADER *trader, BRS_PACKET_HEADER *pkt, void *data){
    pthread_mutex_lock(&trader->mutex);

    int type = pkt->type;

    if(type==BRS_ACK_PKT){
        debug("Send packet (clientfd=%d, type=ACK) for trader [%s]", trader->fd, trader->name);
        //trader_send_ack(trader,data);
    }
    else{
        debug("Send packet (clientfd=%d, type=NACK) for trader [%s]", trader->fd, trader->name);
        //trader_send_nack(trader);
    }


    pthread_mutex_unlock(&trader->mutex);

    return 0;
}

/*
 * Broadcast a packet to all currently logged-in traders.
 *
 * @param pkt  The packet to be sent.
 * @param data  Data payload to be sent, or NULL if none.
 * @return 0 if broadcast succeeds, -1 otherwise.
 */
int trader_broadcast_packet(BRS_PACKET_HEADER *pkt, void *data);

/*
 * Send an ACK packet to the client for a trader.
 *
 * @param trader  The TRADER object for the client who should receive
 * the packet.
 * @param infop  Pointer to the optional data payload for this packet,
 * or NULL if there is to be no payload.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int trader_send_ack(TRADER *trader, BRS_STATUS_INFO *info){
    /*pthread_mutex_lock(&trader->mutex);
    int fd = trader->fd;
    int size = sizeof(BRS_STATUS_INFO);

    proto_send_packet(fd, BRS_PACKET_HEADER *hdr, void *payload);

    pthread_mutex_unlock(&trader->mutex);*/
    return 0;
}

/*
 * Send an NACK packet to the client for a trader.
 *
 * @param trader  The TRADER object for the client who should receive
 * the packet.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int trader_send_nack(TRADER *trader);

/*
 * Increase the balance for a trader.
 *
 * @param trader  The trader whose balance is to be increased.
 * @param amount  The amount by which the balance is to be increased.
 */
void trader_increase_balance(TRADER *trader, funds_t amount){

    pthread_mutex_lock(&trader->mutex);
    trader->balance = trader->balance + amount;
    debug("increase balance: %d->%d",trader->balance-amount,trader->balance);
    pthread_mutex_unlock(&trader->mutex);
}

/*
 * Attempt to decrease the balance for a trader.
 *
 * @param trader  The trader whose balance is to be decreased.
 * @param amount  The amount by which the balance is to be decreased.
 * @return 0 if the original balance is at least as great as the
 * amount of decrease, -1 otherwise.
 */
int trader_decrease_balance(TRADER *trader, funds_t amount){

    pthread_mutex_lock(&trader->mutex);
    funds_t balance = trader->balance;
    if(balance>=amount){
        trader->balance = balance - amount;
        debug("decrease balance: %d->%d",balance,trader->balance);
        pthread_mutex_unlock(&trader->mutex);
        return amount;
    }
    else{
        pthread_mutex_unlock(&trader->mutex);
        return -1;
    }

}

/*
 * Increase the inventory for a trader by a specified quantity.
 *
 * @param trader  The trader whose inventory is to be increased.
 * @param amount  The amount by which the inventory is to be increased.
 */
void trader_increase_inventory(TRADER *trader, quantity_t quantity){

    pthread_mutex_lock(&trader->mutex);
    trader->inventory = trader->inventory + quantity;
    debug("increase quantity: %d->%d",trader->inventory-quantity,trader->inventory);
    pthread_mutex_unlock(&trader->mutex);
}

/*
 * Attempt to decrease the inventory for a trader by a specified quantity.
 *
 * @param trader  The trader whose inventory is to be decreased.
 * @param amount  The amount by which the inventory is to be decreased.
 * @return 0 if the original inventory is at least as great as the
 * amount of decrease, -1 otherwise.
 */
int trader_decrease_inventory(TRADER *trader, quantity_t quantity){

    pthread_mutex_lock(&trader->mutex);
    quantity_t inventory = trader->inventory;
    if(inventory>=quantity){
        trader->inventory = inventory - quantity;
        debug("decrease quantity: %d->%d",inventory,trader->inventory);
        pthread_mutex_unlock(&trader->mutex);
        return quantity;
    }
    else{
        pthread_mutex_unlock(&trader->mutex);
        return -1;
    }
}





