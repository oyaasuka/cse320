#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include<sys/wait.h>

char* get_opt(char* buf);

char* get_task(char* buf);

int get_id(char* buf);

void handler();

int handler_function();
void child_handler_function();

//char* hmsg = "Available commands:\nhelp (0 args) Print this help message\nquit (0 args) Quit the program\n";


