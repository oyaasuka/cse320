#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();



    sf_malloc(4016);
    sf_show_heap();


    // After realloc'ing x, we can return a block of size 48 to the freelist.
    // This block will go into the main freelist and be coalesced.
    //printf("pass:%p\n", y);


    sf_mem_fini();

    return EXIT_SUCCESS;
}
