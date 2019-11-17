/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jobber.h"
#include "task.h"
#include "helper.h"




typedef struct job {
    int id;
    char *task;
    JOB_STATUS status;
    int J_occupied;
    int R_occupied;
} job;

typedef struct job_table {
    int size;
    int length;
    int enabled;
    struct job elements[MAX_JOBS];
} job_table;

typedef struct run_table {
    int size;
    int length;
    struct job elements[MAX_RUNNERS];
    char ** task_list;
} run_table;

static struct job_table J_table;
static struct run_table R_table;

int if_JT_fulled();
int if_RT_fulled();
void jobs_start();
void job_starts(int id, PIPELINE_LIST* pipelines);
int task_start(int id, PIPELINE_LIST* pipelines);
int pipeline_execute(PIPELINE *p);
job get_job(int id);
#define MAXARGS (64)




int jobs_init(void) {
    // TO BE IMPLEMENTED
    sf_set_readline_signal_hook(handler_function);
    J_table.size = MAX_JOBS;
    J_table.length = 0;
    R_table.size = MAX_RUNNERS;
    R_table.length = 0;
    R_table.task_list =(char **)malloc(MAX_RUNNERS*sizeof(char *));
    if(signal(SIGCHLD,handler) == SIG_ERR){
        fprintf(stderr, "%s: %s\n", "signal_error", strerror(errno));
        return -1;
    }

    return 0;
}

void jobs_fini(void) {
    free(R_table.task_list);
}

int jobs_set_enabled(int val) {
    int prev = J_table.enabled;

    J_table.enabled = val;
    if(jobs_get_enabled()!=0)
        jobs_start();

    return prev;
}

int jobs_get_enabled() {
    return J_table.enabled;
}

int job_create(char *command) {
    // TO BE IMPLEMENTED
    printf("Task: %s\n", command);

    int i = 0;
    while(i<MAX_JOBS){
        if(J_table.elements[i].J_occupied==0){
            break;
        }
        else{
            i++;
        }
    }

    if(i == MAX_JOBS) return -1;

    sf_job_create(i);
    J_table.elements[i].id = i;
    J_table.elements[i].status = NEW;
    J_table.elements[i].task = command;
    J_table.elements[i].J_occupied = 1;
    J_table.length+=1;
    J_table.elements[i].status = WAITING;
    sf_job_status_change(J_table.elements[i].id, NEW, WAITING);
    return J_table.elements[i].id;
}

int job_expunge(int jobid) {
    job j = get_job(jobid);
    if(j.status==COMPLETED||j.status==ABORTED){
        j.R_occupied=0;
        j.J_occupied=0;
        sf_job_expunge(jobid);
        return 0;

    }
    return -1;
}


int job_cancel(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_pause(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_resume(int jobid) {
    abort();
}

int job_get_pgid(int jobid) {
    return jobid;
}

JOB_STATUS job_get_status(int jobid) {
    // TO BE IMPLEMENTED
    job j =get_job(jobid);
    JOB_STATUS status = j.status;
    char* s;
    if(status== NEW)
        s = "new";
    else if(status== WAITING)
        s = "waiting";
    else if(status== RUNNING)
        s = "running";
    else if(status== PAUSED)
        s = "paused";
    else if(status== CANCELED)
        s = "canceled";
    else if(status== COMPLETED)
        s = "completed";
    else
        s = "aborted";

    printf("job %d [%s]: %s\n", jobid,s, j.task);
    return j.status;
}

int job_get_result(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_was_canceled(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

char *job_get_taskspec(int jobid) {

    return get_job(jobid).task;
}

int if_JT_fulled(){
    for(int i=0;i<MAX_JOBS;i++){
        if(!J_table.elements[i].J_occupied)
            return 0;
    }
    return 1;
}

int if_RT_fulled(){
    for(int i=0;i<MAX_RUNNERS;i++){
        if(!R_table.elements[i].R_occupied)
            return 0;
    }
    return 1;
}


job get_job(int id){
    for(int i=0; i<MAX_JOBS;i++){
        if(J_table.elements[i].id == id&&J_table.elements[i].J_occupied)
            return J_table.elements[i];
    }
    abort();
}

void jobs_start(){// start waiting job in run-table

    if(!if_RT_fulled()){// put jobs in running table

        for(int i=0,j=R_table.length;i<MAX_JOBS;i++){
            if(j>=MAX_RUNNERS)
                break;
            if(J_table.elements[i].J_occupied && !J_table.elements[i].R_occupied ){
                R_table.elements[j] =J_table.elements[i];
                R_table.elements[j].id =J_table.elements[i].id;
                R_table.elements[j].task =J_table.elements[i].task;
                R_table.elements[j].status =J_table.elements[i].status;
                R_table.elements[j].R_occupied =1;
                R_table.task_list[j] = job_get_taskspec(i);
                R_table.length++;
                j++;

            }


        }
    }

    for(int i=0;i<MAX_RUNNERS;i++){
        char **tmp =(char **)malloc(sizeof(char *));
        tmp[0] = *(R_table.task_list+i);
        if(R_table.elements[i].R_occupied&&R_table.elements[i].status==WAITING){
            TASK* tasks = parse_task(tmp);
            PIPELINE_LIST* pipelines = tasks->pipelines;
            job_starts(R_table.elements[i].id, pipelines);
        }
        free(tmp);
    }
}

void job_starts(int id, PIPELINE_LIST* pipelines){ // run a single job
    pid_t pid;
    job j;
    for(int i = 0;i<MAX_JOBS;i++){
        if(J_table.elements[i].id == id &&J_table.elements[i].J_occupied==1)
             j=J_table.elements[i];
    }
    j.status = RUNNING;
    int status;
    int exit_s;

    if((pid=fork())==0){//create a running process
        setpgid(0,0); // create a new running group
        sf_job_start(j.id, getpgrp());
        if(task_start(j.id,pipelines)==0) exit(EXIT_SUCCESS);
        else abort();
    }


    waitpid(-1,&status,0);
    if(WIFEXITED(status)){
            exit_s = WEXITSTATUS(status);
    }
    else{
        abort();
    }

    sf_job_end(j.id, getpgrp(),exit_s);
    j.status = COMPLETED;
    sf_job_status_change(j.id, RUNNING,COMPLETED);



}

int task_start(int jobid, PIPELINE_LIST* pipelines){//execute pipe sequentially
    pid_t pid;
    int status;
    int exit_s;
    sf_job_status_change(jobid, WAITING, RUNNING);
    PIPELINE_LIST* header = pipelines;
    while(header!=NULL){
        if((pid=fork())==0){// pipeline master process
            if(pipeline_execute(header->first)==0){
                exit(EXIT_SUCCESS);
            }
            else{
                abort();
            }
        }

        waitpid(-1,&status,0);

        if(WIFEXITED(status)){
            exit_s = WEXITSTATUS(status);
        }
        else{
            abort();
        }

        /*if((result=waitpid(-1, &status, 0))!=-1 &&header ->rest==NULL){
            if(!WIFEXITED(status)) abort();
            sf_job_end(jobid, job_get_pgid(jobid), result);
            job j =get_job(jobid);
            j.status = COMPLETED;
            sf_job_status_change(jobid, RUNNING,j.status);
        }*/
        header = header ->rest;

    }

    return exit_s;

}



int pipeline_execute(PIPELINE *p){
    int fd[2];
    pid_t pid;
    int fd_in = 0;
    int has_out;
    int out;
    int in;
    //int status;
    //int exit_s;
    if(p->input_path!= NULL){
        in = open(p->input_path, O_RDONLY);
        fd_in = in;
    }
    else if(p->output_path!= NULL){
        has_out=1;
        out = open(p->output_path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP| S_IWUSR);
    }


    COMMAND_LIST * commands_header = p->commands;
    while(commands_header != NULL){

        COMMAND* command = commands_header->first;
        WORD_LIST* header = command->words;
        char *argv[MAXARGS];
        int i =0;
        while(header!=NULL){
            argv[i] = header->first;;
            header = header ->rest;
            i++;
        }
        argv[i] = NULL;

        pipe(fd);

        if((pid=fork())==-1){
            exit(EXIT_FAILURE);
        }

        if(pid==0){
            dup2(fd_in, 0);
            if(commands_header->rest!=NULL)
                dup2(fd[1],1);
            else{
                if(has_out){
                    dup2(out,1);
                    close(out);
                }
            }

            close(fd[0]);
            execvp(argv[0],argv);
            exit(EXIT_FAILURE);

        }

        /*waitpid(-1, &status, 0);

        if(WIFEXITED(status)){
            exit_s = WEXITSTATUS(status);
        }
        else{
            abort();
        }*/
        close(fd[1]);
        fd_in = fd[0];
        commands_header = commands_header->rest;
    }
    return 0;

}







