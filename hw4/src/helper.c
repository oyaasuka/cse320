#include "helper.h"
#include <string.h>

static int child_signal_arrived;

char* get_opt(char* buf){
    char* opt;
    int i=0;
    while(*buf!='\0'){
        if(*(buf+i)==' '||*(buf+i)=='\0'){
        *(buf+i)='\0';
        break;
        }
    i++;
    }
  opt = buf;
  return opt;

}

char* get_task(char* buf){
    char* task;

    while(1){
        if(*(buf++)=='\''){
        buf[strlen(buf)-1] = '\0';
        break;
        }
    }
    task = buf;
    return task;
}

int handler_function(){
    if(child_signal_arrived==1)
        child_handler_function();

    return 1;

}

void handler(){
    child_signal_arrived = 1;
}

void child_handler_function(){
    //pid_t pid;
    child_signal_arrived=0;
    waitpid(-1, NULL, 0);
        //printf("handler reaped child%d", (int)pid);
    return;
}











