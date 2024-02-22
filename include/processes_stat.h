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
    long proc_elapsedSec;
    long proc_usgInSec;
    char state;
    char* command;
};



void running_processes(Array procArray);

void sys_cpu_usage();

void assign_process();

void stat_parsing(Proc currNewProc, char* currEntry);

void statm_parsing(Proc currNewProc, char* currEntry);

void cmdline_parsing(Proc currNewProc, char* currEntry);

void proc_cpu_usage(Proc currProc);

char* get_user_from_euid(uid_t euid);

char* strformat_ms_time(long totalSeconds);

int is_pid_dir(const struct dirent *entry);

int pid_exist(Array procArray, const struct dirent *entry);



#endif