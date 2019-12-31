#include <stdio.h>
#include <stdlib.h>

/*to count the length of a string*/
int strLen(char *S);

int streq(char *a, char *b);

void create_magic_bits();

void create_depth_bits(int depth);

void create_size_bits(long size);

void create_header(int type, int depth);

void directory_entry(int depth, mode_t mode, off_t size, char* filename);

void create_metadata_bits();

void create_filename_bits(char* filename);

void file_data(int depth, off_t size);

int get_record();

int get_se_transmission_bits(int type);

int get_magic_bits(char c);

int check_start_directory(int type, int depth, char c);

