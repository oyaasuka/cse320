#include "const.h"
#include "transplant.h"
#include "debug.h"
/*below is I add*/
#include <stdlib.h>
#include "template.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
	return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
	return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
	return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
	return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
	return "DIRECTORY_ENTRY";
    case FILE_DATA:
	return "FILE_DATA";
    default:
	return "UNKNOWN";
    }
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    // To be implemented.
    int lenOfName = strLen(name);
    char *p = path_buf;
    
    if(lenOfName >= (sizeof(path_buf)/sizeof(*p))){
        return -1;
    }

    else{
        while(*name !='\0'){
            *(p++) = *(name++);     
        }
        *p = '\0';
        path_length = lenOfName;
        return 0;   
    }
}



/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    // To be implemented.
    int lenOfName = strLen(name);
    char *p = path_buf;
    
    if(path_length+lenOfName+1 >= (sizeof(path_buf)/sizeof(*p))){
        return -1;    
    }  
    else{
        path_buf[path_length] = '/';
        
        p+=path_length;
        p++;

        while(*name!='\0'){
            if(*name == '/')
                return -1;
            *(p++) = *(name++);         
        }
        *p = '\0';    
        
        path_length += lenOfName;
        path_length++;
        
        return 0; 
    }
    
    
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    // To be implemented.
    char *p0 = path_buf;
    if(path_length == 0||*p0 =='\0'){
        return -1;    
    }
    else{
        char *p1;
        p1 = p0;
        p1+=path_length;
        p1--;

        while(p1!=p0 && *p1 !='/'){
            path_length--;
            p1--;
        }
        path_length--;
        *p1 = '\0';

        
        return 0;     
    }
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    // To be implemented.
    int dep;
    char first = getchar();
    
    if(first == EOF)
        return 0;
    
    if(check_start_directory(START_OF_DIRECTORY,depth, first))
        return -1;
    
    
    
    char c;
    int i, size_of_record, mode;
    
    
    
    
    while(1){
        dep=0;
        size_of_record = 0;
        mode =0;
        // to check data entry
        
        // check magic code
        c = getchar();
        if(c == EOF || c != '\x0c')
            return -1;
    
        c = getchar();
        if(c == EOF || c!='\x0d') 
            return -1;
    
        c = getchar();
        if(c == EOF || c !='\xed') 
            return -1;
        // check type, if 03 type jump out loop
        c = getchar();
        if(c == END_OF_DIRECTORY)
            break;
        if(c == EOF || c != DIRECTORY_ENTRY)
            return -1;
        
        //check depth
        for(i=0;i<4;i++){
            c = getchar();
        
            if(c == EOF)
                return -1;
        
            dep += ((c&0xff)<<(24-i*8));
        } 
        if(dep != depth)
            return -1;
        
        //count total size of this record
        
        for(i=0;i<8;i++){
            c = getchar();
        
            if(c == EOF)
                return -1;
        
            size_of_record += ((c&0xff)<<(56-i*8));
        } 
        if(size_of_record <= 28)
            return -1;
        
        // to record the mode and judge
        
        for(i=0;i<4;i++){
            c = getchar();
        
            if(c == EOF)
                return -1;
        
            mode += ((c&0xff)<<(24-i*8));
            
        } 
        
        for(i=0;i<8;i++)
            getchar();
            
        char *p = name_buf;

        for(i=28;i<size_of_record;i++){
            *(p++) = getchar();
        }
        *p = '\0';
            
        path_push(name_buf);
    
        if(S_ISREG(mode)){
            if(deserialize_file(depth))
                return -1;
            path_pop();
            
        }
        else if(S_ISDIR(mode)){
            struct stat buf;
            if(stat(path_buf, &buf) == 0){
                if(((global_options >>3)&1) == 0){
                    return -1;
                }
            }
            mkdir(path_buf, 0700);
            chmod(path_buf, mode & 0777);
            deserialize_directory(depth+1);
        }
        
        
        
        
          
    }
    dep = 0;
    size_of_record = 0;
    
    for(i=0;i<4;i++){
        c = getchar();
        
        if(c == EOF)
            return -1;
        
        dep += ((c&0xff)<<(24-i*8));
    } 
    
    if(dep != depth)
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

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth){
    char c;  
    int i, size_of_record =0, dep =0; 


    // check magic code
        c = getchar();
        if(c == EOF || c != '\x0c')
            return -1;
    
        c = getchar();
        if(c == EOF || c!='\x0d') 
            return -1;
    
        c = getchar();
        if(c == EOF || c !='\xed') 
            return -1;

    // check type
        c = getchar();
        if(c == EOF || c != FILE_DATA)
            return -1;

    //check depth
        for(i=0;i<4;i++){
            c = getchar();
        
            if(c == EOF)
                return -1;
        
            dep += ((c&0xff)<<(24-i*8));
        } 
        if(dep != depth)
            return -1;

    //count total size of this record
        
        for(i=0;i<8;i++){
            c = getchar();
        
            if(c == EOF)
                return -1;
        
            size_of_record += ((c&0xff)<<(56-i*8));
        } 
        if(size_of_record < 16)
            return -1;

    //check if file exist
    struct stat buf;
    if(stat(path_buf, &buf) == 0){
        if(((global_options >>3)&1) == 0){
            return -1;
        }
    }
        

    

    FILE *f = fopen(path_buf, "w");
    for(i=0;i<(size_of_record-16);i++){
        fputc(getchar(),f);
    }
    

    fclose(f);
    
    return 0;
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth) {
    // To be implemented.
    struct dirent *dp;
    DIR *dir = opendir(path_buf);
    struct stat stat_buf;
    

    create_header(START_OF_DIRECTORY, depth);
    
    while((dp = readdir(dir))!=NULL){
        char *curname = dp->d_name;
        
        if(streq(curname, ".") || streq(curname, ".."))
            continue;
        
        path_push(curname);
        stat(path_buf, &stat_buf);
        
        if(S_ISREG(stat_buf.st_mode)){
            directory_entry(depth, stat_buf.st_mode, stat_buf.st_size, curname);
            
            if(serialize_file(depth,stat_buf.st_size))
                return -1;
            
            path_pop();
            continue;    
        }
        else if(S_ISDIR(stat_buf.st_mode)){
            directory_entry(depth, stat_buf.st_mode, stat_buf.st_size, curname);
            serialize_directory(depth+1);
        }
        else{
            path_pop();
            return -1;
        }

    }
    
    create_header(END_OF_DIRECTORY, depth);
    
    closedir(dir);
    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {
    // To be implemented.
    FILE *f = fopen(path_buf, "r");

    if(f==NULL)
        return -1;

    file_data(depth, size);
    char ch = fgetc(f);
    while(ch != EOF){
        putchar(ch);
        ch = fgetc(f);
    }
    
    
    fclose(f);
    return 0;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {
    // To be implemented.
    char *p =path_buf;
    if( *p== '\0'){
        path_init(".");
    }
    
    create_header(START_OF_TRANSMISSION, 0);
    if(serialize_directory(1))
        return -1;
    create_header(END_OF_TRANSMISSION, 0);
    fflush(stdout);
    return -1;
}

/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */

int deserialize() {
    // To be implemented.
    char *p =path_buf;
    if( *p== '\0'){
        path_init(".");
    }

    DIR *d = opendir(path_buf);
    if(d){
        
        closedir(d);
    }
    else{
        mkdir(path_buf, 0700);
    }
    
    if(get_se_transmission_bits(START_OF_TRANSMISSION))
        return -1;
    
    if(deserialize_directory(1))
        return -1;
    
    
    if(get_se_transmission_bits(END_OF_TRANSMISSION))
        return -1;
    
    return 0;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    // To be implemented.
    if(streq(*(argv+1), "-h")){
        global_options |= (1u << 0);
        return 0;
    }

    else if(streq(*(argv+1), "-s")){
        argv++;

        int i =1;
        while(i != argc){
            
            char *cur_flag = *argv;
            
            if(streq(cur_flag, "-c")){
                return -1;
            }
            if(streq(cur_flag, "-p")){//note

                if(i>= argc -1){
                    return -1;
                }

                if(**(argv+1) != '-'){
                    //printf("%s\n", *(argv+1));
                    path_init(*(argv+1));// or remember to get current directory!
                }
                else{
                    return -1;
                }
            }
            
            //printf("%s %d\n", *argv, i); 
            argv++;
            i++;
        }
        global_options |= (1u << 1);
        return 0;
    }
    
    else if(streq(*(argv+1), "-d")){
        argv++;

        int i =1;
        while(i != argc){
            
            char *cur_flag = *argv;
            
            if(streq(cur_flag, "-c")){
                global_options |= (1u << 3);
            }
            if(streq(cur_flag, "-p")){//note

                if(i>= argc -1){
                    global_options &= ~(1u << 3);
                    return -1;
                }

                if(**(argv+1) != '-'){
                    //printf("%s\n", *(argv+1));
                    path_init(*(argv+1));// or remember to get current directory!
                }
                else{ 
                    global_options &= ~(1u << 3);
                    return -1;
                }
            }
            argv++;
            i++;
        }

        global_options |= (1u << 2);
        return 0;
    }
    
    return -1;
}
