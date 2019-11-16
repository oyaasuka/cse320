/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include<sys/wait.h>

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
void jobs_starts();
void job_execute(int id, PIPELINE_LIST* pipelines);
void task_start();
void pipeline_execute(PIPELINE *p);
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
        exit(0);
    }

    return -1;
}

void jobs_fini(void) {
    // TO BE IMPLEMENTED
    abort();
}

int jobs_set_enabled(int val) {
    int prev = jobs_get_enabled();
    //int i;
    J_table.enabled = val;

    if(!if_RT_fulled()){

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

        jobs_starts();



    }


    //sleep(1);q

    return prev;
}

int jobs_get_enabled() {
    return J_table.enabled;
}

int job_create(char *command) {
    // TO BE IMPLEMENTED
    printf("Task: %s\n", command);

    int i = 0;
    while(i<8){
        if(J_table.elements[i].J_occupied==0){
            break;
        }
        else{
            i++;
        }
    }

    sf_job_create(i);
    J_table.elements[i].id = i;
    J_table.elements[i].status = NEW;
    J_table.elements[i].task = command;
    J_table.elements[i].J_occupied = 1;
    J_table.length+=1;
    J_table.elements[i].status = WAITING;
    sf_job_status_change(J_table.elements[i].id, NEW, WAITING);
    return 1;
}

int job_expunge(int jobid) {
    // TO BE IMPLEMENTED
    abort();
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
    abort();
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

void job_execute(int id, PIPELINE_LIST* pipelines){
    pid_t pid;
    job j = get_job(id);
    j.status = RUNNING;

    if((pid=fork())==0){//create a running process
        setpgid(0,0); // create a new running group
        //sf_job_start(getpid(), job_get_pgid(j.id));
        //sf_job_status_change(j.id, WAITING, RUNNING);
        task_start(j.id);
        exit(0);
    }

}

void task_start(int jobid){
    for(int i =0;i<MAX_RUNNERS;i++){
        if((R_table.elements[i].R_occupied ==1) && (R_table.elements[i].id == jobid)){
            TASK* task = parse_task(R_table.task_list+i);
            PIPELINE_LIST* header = task->pipelines;
            while(header!=NULL){
                pipeline_execute(header->first);
                header = header ->rest;
            }
        }

    }

}

job get_job(int id){
    for(int i=0; i<MAX_JOBS;i++){
        if(J_table.elements[i].id == id)
            return J_table.elements[i];
    }
    abort();
}

void jobs_starts(){
    for(int i=0;i<MAX_RUNNERS;i++){
        char **tmp =(char **)malloc(sizeof(char *));
        tmp[0] = *(R_table.task_list+i);
        if(R_table.elements[i].R_occupied&&R_table.elements[i].status==WAITING){
            TASK* tasks = parse_task(tmp);
            PIPELINE_LIST* pipelines = tasks->pipelines;
            job_execute(R_table.elements[i].id, pipelines);
        }
        free(tmp);
    }
}

void pipeline_execute(PIPELINE *p){
    if(p->input_path!= NULL){

    }
    else if(p->output_path!= NULL){

    }
    else{
        COMMAND_LIST * commands = p->commands;
        if(commands->rest == NULL){
            COMMAND* command = commands->first;
            WORD_LIST* header = command->words;
            char *argv[MAXARGS];
            int i =0;
            while(header!=NULL){
                argv[i] = header->first;;
                header = header ->rest;
                i++;
            }
            argv[i] = NULL;
            execvp(*argv,argv);

        }
    }
}




