#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "jobber.h"
#include "sf_readline.h"
#include "helper.h"
#include "task.h"


/*
 * "Jobber" job spooler.
 */

#define ENABLED 1
#define UNENABLED 0


int main(int argc, char *argv[])
{
    // TO BE IMPLEMENTED
    //
    jobs_init();
    char* buf = sf_readline("jobber> ");
    char* opt = get_opt(buf);


    if(strcmp(opt, "spool")==0){
        job_create(get_task(buf));
    }

    buf = sf_readline("jobber> ");
    opt = get_opt(buf);
    if(strcmp(opt, "spool")==0){
        job_create(get_task(buf));
    }

    buf = sf_readline("jobber> ");
    opt = get_opt(buf);
    if(strcmp(opt, "enable")==0){
        jobs_set_enabled(ENABLED);
    }

    sf_readline("jobber> ");


    /*int n = strlen("echo start ; cat /etc/passwd | grep bash > out ; echo done");
    char *tp = "echo start ; cat /etc/passwd | grep bash > out ; echo done";
    char taskcpy[n];

    for(int i = 0; i<n; i++){
        taskcpy[i] = *(tp+i);
    }
    taskcpy[n] = '\0';
    tp = taskcpy;

    n = strlen("echo start");
    char *tp1 = "echo start";
    char taskcpy1[n];

    for(int i = 0; i<n; i++){
        taskcpy1[i] = *(tp1+i);
    }
    taskcpy1[n] = '\0';
    tp1 = taskcpy1;



    char **array;
    int row;
    row =2;
    array=(char **)malloc(row*sizeof(char *));

        array[0]=tp;
        array[1]=tp1;*/






    //char *b = *a;
    //*b = '\0';
    //printf("%c\n");
    /*TASK *test_tasks = parse_task(array);
    PIPELINE_LIST *test_pipelines = test_tasks->pipelines;
    PIPELINE * test_pipeline = test_pipelines->first;
    COMMAND_LIST * test_commands = test_pipeline->commands;
    COMMAND * test_command = test_commands ->first;
    WORD_LIST* test_words = test_command->words;
    char* test_word = test_words ->first;

    printf("%s\n",test_word);*/


    exit(EXIT_FAILURE);
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
