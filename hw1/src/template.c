#include <stdio.h>
#include <stdlib.h>
#include "template.h"
#include "transplant.h"

/*to count the length of a string*/
int strLen(char *S){
    int lenOfS = 0;

    /*for(i = 0;;i++){
        if(S[i] == '\0')
            break;
        else
            lenOfS++;
    }*/

    while(*(S++)!='\0'){
        lenOfS++;   
    }
    

    return lenOfS;
}

int streq(char *x, char *y){
    while(*x){
        if(*x!=*y){
            return 0;
        }
        x++;
        y++;   
    }

    return -1;
        
}

void create_magic_bits(){
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
}

void create_depth_bits(int depth){
    int i;
    for(i =0;i<4;i++){
        putchar((depth>>(24-i*8)&0xff));
    }
}

void create_size_bits(long size){
    int i;
    for(i =0;i<8;i++){
        putchar((size>>(56-i*8)&0xff));
    }
}

void create_header(int type, int depth){
    create_magic_bits();
    putchar(type);
    create_depth_bits(depth);
    
    long size = 8;
    size += 8;
    create_size_bits(size);
}

void directory_entry(int depth, mode_t mode, off_t st_size, char* filename){
    create_magic_bits();
    putchar(4);
    create_depth_bits(depth);
    
    long size = 8;
    size += 20;
    
    int file_length = 0;
    char *p = filename;
    
    while(*(p++)!='\0'){
        file_length++;    
    }

    size += file_length;
    
    create_size_bits(size);

    create_metadata_bits(mode, st_size);
    
    create_filename_bits(filename);
}

void create_metadata_bits(mode_t mode, off_t st_size){

    int i;
    for(i =0;i<4;i++){
        putchar((mode>>(24-i*8)&0xff));
    }

    for(i =0;i<8;i++){
        putchar((st_size>>(56-i*8)&0xff));
    }

}

void create_filename_bits(char* filename){
    
    while(*filename !='\0'){

        putchar(*filename);
        filename++;
    }

}

void file_data(int depth, off_t st_size){
    create_magic_bits();
    putchar(5);
    create_depth_bits(depth);
    
    long size = 8;
    size += 8;
    size += st_size;
    create_size_bits(size);
}

int get_magic_bits(char c){
    
    if(c != 0x0c)
        return -1;
    if(getchar() != 0x0d)
        return -1;
    if(getchar() != 0xed)
        return -1;
    
    
    return 0;
}

int get_se_transmission_bits(int type){
    int t, i;
    char c = getchar();

    if(c == EOF || c != '\x0c')
        return -1;
    
    c = getchar();
    if(c == EOF || c!='\x0d')
        return -1;
    
    c = getchar();
    if(c == EOF || c !='\xed') 
        return -1;

    t = getchar();

    if(type != t)
        return -1;

    int dep = 0;
    int size_of_record = 0;
    
    for(i=0;i<4;i++){
        c = getchar();
        
        if(c == EOF)
            return -1;
        
        dep += ((c&0xff)<<(24-i*8));
    } 
    
    if(dep != 0)
        return -1;
    
    for(i=0;i<8;i++){
        c = getchar();
        
        if(c == EOF)
            return -1;
        
        size_of_record += ((c&0xff)<<(56-i*8));
    } 
    if(size_of_record != 16)
            return -1;
    return 0;
}


int check_start_directory(int type, int depth, char first){
    int i;
    char c;
    int dep=0;

    if(first != '\x0c')
        return -1;
    
    c= getchar();
    if(c == EOF || c!='\x0d') 
        return -1;
    
    c = getchar();
    if(c !='\xed') 
        return -1; 

    
    if(getchar()!=type)
        return -1;
    
    for(i=0;i<4;i++){
        c = getchar();
        
        if(c == EOF)
            return -1;
        
        dep += (c<<(24-i*8));
    } 

    if(dep!= depth)
        return -1;
    
    for(i=0;i<7;i++){
        if(getchar()==EOF)
            return -1;
    }
    
    if(getchar()!=16)
        return -1;

    return 0;
}

