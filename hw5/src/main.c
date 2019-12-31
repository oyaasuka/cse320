 #include <stdlib.h>

#include "client_registry.h"
#include "exchange.h"
#include "trader.h"
#include "debug.h"
#include "csapp.h"
#include "server.h"

extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;

static void terminate(int status);
void sighup_handler(int sig){terminate(EXIT_SUCCESS);}

/*
 * "Bourse" exchange server.
 *
 * Usage: bourse <port>
 */
int main(int argc, char* argv[]){
    int listenfd, *connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientadder;
    pthread_t tid;
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    if(argc !=2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
    }
    port = atoi(argv[1]);


    // Perform required initializations of the client_registry,
    // maze, and player modules.
    client_registry = creg_init();
    exchange = exchange_init();
    trader_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function brs_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    Signal(SIGHUP,sighup_handler);

    if((listenfd = Open_listenfd(port))>0) debug("Bourse server listening on port %s", argv[1]);

    while(1){
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientadder,&clientlen);
        Pthread_create(&tid, NULL, brs_client_service, connfd);
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the Bourse server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}
