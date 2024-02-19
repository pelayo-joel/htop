#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../include/processes_stat.h"
#include "../include/utilities.h"



Array running_processes() {
    Array running_processes = malloc(sizeof(struct array));
    struct dirent* dirEntry;
    DIR* srcdir = opendir("/proc");
  
    init_array(running_processes);
    while((dirEntry = readdir(srcdir)) != NULL) {
        if (!is_pid_dir(dirEntry)) continue;
        
        Proc currProc = malloc(sizeof(struct process));                    
        currProc->PID = atoi(dirEntry->d_name);
        stat_parsing(currProc, dirEntry->d_name);
        statm_parsing(currProc, dirEntry->d_name);
        cmdline_parsing(currProc, dirEntry->d_name);

        push_back(running_processes, (void*)currProc, sizeof(struct process));
    }
    return running_processes;
}

// long total_mem() {
//     FILE* meminfo = fopen("/proc/meminfo");
// }

// long total_cpu() {
//     FILE* cpuinfo = fopen("/proc");
// }


void stat_parsing(Proc currNewProc, char* currEntry) {
    int fcolumn = 1;
    char* token;
    char fileStrBuf[2048];
    char statPath[256];
    FILE* statFile;

    snprintf(statPath, sizeof(statPath), "/proc/%s/stat", currEntry);
    statFile = fopen(statPath, "r");
    fgets(fileStrBuf, 2048, statFile);

    token = strtok(fileStrBuf, " ");
    while (token) {
        if (fcolumn == 3) currNewProc->state = token[0];
        if (fcolumn == 18) currNewProc->priority = atoi(token);
        if (fcolumn == 19) currNewProc->niceness = atoi(token);
        if (fcolumn == 13) currNewProc->proc_utime = strtol(token, NULL, 0);
        if (fcolumn == 14) currNewProc->proc_stime = strtol(token, NULL, 0);
        if (fcolumn == 21) {
            currNewProc->proc_startTime = strtol(token, NULL, 0);
            break;
        }
        token = strtok(NULL, " ");
        fcolumn++;
    }
    fclose(statFile);   
}

void statm_parsing(Proc currNewProc, char* currEntry) {
    FILE* statmFile;
    char memPath[512];

    snprintf(memPath, sizeof(memPath), "/proc/%s/statm", currEntry);
    statmFile = fopen(memPath, "r");
    fscanf(statmFile, "%ld %ld %ld", &currNewProc->virt_usg, &currNewProc->res_usg, &currNewProc->shr_usg);
    
    currNewProc->virt_usg *= 4;
    currNewProc->res_usg *= 4;
    currNewProc->shr_usg *= 4;
}

void cmdline_parsing(Proc currNewProc, char* currEntry) {
    char cmdPath[128];
    char cmdlineBuf[512];
    FILE* cmdlineFile;

    snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", currEntry); 
    cmdlineFile = fopen(cmdPath, "rb");

    currNewProc->command = malloc(sizeof(char*) * 512);
    int ns = fscanf(cmdlineFile, "%[^\n]", cmdlineBuf);
    for (int i = 0; i < ns; i++) {
        if (!cmdlineBuf[i] && cmdlineBuf[i] == '\0') cmdlineBuf[i] = ' '; 
    }
    strcpy(currNewProc->command, cmdlineBuf);
    fclose(cmdlineFile);
}

// char* strformat_hms_time(long seconds) {
//     char* hmsTime = malloc(sizeof(char) * 128);
//     snprintf(hmsTime, "%ld", seconds);
//     return hmsTime;
// }

int is_pid_dir(const struct dirent *entry) {
    const char *p;
    for (p = entry->d_name; *p; p++) {
        if (!isdigit(*p)) return 0;
    }
    return 1;
}