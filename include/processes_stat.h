#ifndef PROCESSES_STAT_H
#define PROCESSES_STAT_H

#include <stdio.h>
#include <dirent.h>

#include "../include/utilities.h"
#include "../include/processes_stat.h"



typedef struct system* Sys;
typedef struct process* Proc;

struct system {
    
};

struct process {
    int PID;
    int priority;
    int niceness;
    char* processUser;
    long virt_usg;
    long res_usg;
    long shr_usg;
    long proc_utime;
    long proc_stime;
    long proc_startTime;
    char state;
    char* command;
};



Array running_processes();

void assign_process();

void stat_parsing(Proc currNewProc, char* currEntry);

void statm_parsing(Proc currNewProc, char* currEntry);

void cmdline_parsing(Proc currNewProc, char* currEntry);

char* strformat_hms_time(long seconds);

int is_pid_dir(const struct dirent *entry);



#endif