#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv)){
        
        USAGE(*argv, EXIT_FAILURE);}

    debug("Options: 0x%x", global_options);
    if(global_options & 1){
        USAGE(*argv, EXIT_SUCCESS);}
    else if((global_options >>1) & 1){
        if(serialize()==0){
            fflush(stdout);
            return EXIT_SUCCESS;}
        else{
            fflush(stdout);
            return EXIT_FAILURE;}
    }
    else if((global_options >>2) & 1){
        if(deserialize()==0){
            fflush(stdout);
            return EXIT_SUCCESS;}
        else{
            fflush(stdout);
            return EXIT_FAILURE;}
    }
    fflush(stdout);
    return EXIT_FAILURE;
    
}//reminder: to set current directory and delicate the bits

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
