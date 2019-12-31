#include "trader.h"
#include "csapp.h"

typedef struct trader {
    int fd;
    char* name;
    int ref;
    quantity_t inventory;
    funds_t balance;
    TRADER* next;
    int logged;
    BRS_STATUS_INFO * status;
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
}TRADER;

typedef struct map
{
    TRADER* header;
    int size;
    pthread_mutex_t mutex;
}MAP;

