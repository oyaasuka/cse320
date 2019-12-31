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
#define DISABLED 0


int main(int argc, char *argv[])
{
    // TO BE IMPLEMENTED
    //
    if(jobs_init() == -1) exit(EXIT_FAILURE);

    char* buf;
    char *opt;
    while(1) {
        buf = sf_readline("jobber> ");
        opt = get_opt(buf);

        if(strcmp(opt, "spool")==0)
            job_create(get_task(buf));

        if(strcmp(opt, "enable")==0)
            jobs_set_enabled(ENABLED);

        if(strcmp(opt, "disable")==0)
            jobs_set_enabled(DISABLED);

        if(strcmp(opt, "quit") ==0)
            exit(0);

        if(strcmp(opt, "help") ==0){
            printf("Available commands:\n");
            printf("help (0 args) Print this help message\n");
            printf("quit (0 args) Quit the program\n");
            printf("enable (0 args) Allow jobs to start\n");
            printf("disable (0 args) Prevent jobs from starting\n");
            printf("spool (1 args) Spool a new job\n");
            printf("pause (1 args) Pause a running job\n");
            printf("resume (1 args) Resume a paused job\n");
            printf("cancel (1 args) Cancel an unfinished job\n");
            printf("expunge (1 args) Expunge a finished job\n");
            printf("status (1 args) Print the status of a job\n");
            printf("jobs (0 args) Print the status of all jobs\n");
        }

        if(strcmp(opt, "status") ==0){
            job_get_status(get_id(buf));
        }

        if(strcmp(opt, "expunge") ==0){
            job_expunge(get_id(buf));
        }


    }


    jobs_fini();


    exit(EXIT_SUCCESS);
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
