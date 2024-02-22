#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../include/processes_stat.h"
#include "../include/utilities.h"


#define MAX_CORES 32



void running_processes(Array procArray) {
    struct dirent* dirEntry;
    DIR* srcdir = opendir("/proc");
    init_array(procArray, sizeof(struct process));

    while((dirEntry = readdir(srcdir)) != NULL) {
        if (!is_pid_dir(dirEntry)) continue;

        Proc currProc = malloc(sizeof(struct process));                    
        currProc->PID = atoi(dirEntry->d_name);
        stat_parsing(currProc, dirEntry->d_name);
        statm_parsing(currProc, dirEntry->d_name);
        cmdline_parsing(currProc, dirEntry->d_name);

        push_back(procArray, (void*)currProc);
    }
}

void sys_cpu_usage(double* cpu_usage, int num_cores) {
    FILE* file;
    char line[256];

    // Open /proc/stat file
    file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("Error opening /proc/stat file");
        exit(EXIT_FAILURE);
    }

    // Read lines from /proc/stat file
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "cpu", 3) == 0) {
            // Skip the first line which contains overall CPU usage
            if (line[3] == ' ') {
                break;
            }

            int core_id;
            double user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
            sscanf(line, "cpu%d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                   &core_id, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

            // Calculate total CPU time
            double total_time = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;

            // Calculate CPU usage as a percentage
            double usage = (total_time - idle) / total_time * 100;

            // Store CPU usage in the array
            if (core_id >= 0 && core_id < num_cores) {
                cpu_usage[core_id] = usage;
                // memcpy(cpu_usage+(sizeof(double) * (MAX_CORES - 1)), &usage, sizeof(double));
            }
        }
    }

    // Close the file
    fclose(file);
}

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
        if (fcolumn == 2) {
            unsigned int euid = strtoul(token, NULL, 10);
            currNewProc->processUser = get_user_from_euid(euid);
        }
        if (fcolumn == 18) currNewProc->priority = atoi(token);
        if (fcolumn == 19) currNewProc->niceness = atoi(token);
        if (fcolumn == 13) currNewProc->proc_utime = strtol(token, NULL, 0);
        if (fcolumn == 14) currNewProc->proc_stime = strtol(token, NULL, 0);
        if (fcolumn == 21) {
            currNewProc->proc_startTime = strtol(token, NULL, 0);
            proc_cpu_usage(currNewProc);
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
    fclose(statmFile);
}

void cmdline_parsing(Proc currNewProc, char* currEntry) {
    char cmdPath[128];
    char cmdlineBuf[512];
    FILE* cmdlineFile;

    snprintf(cmdPath, sizeof(cmdPath), "/proc/%s/cmdline", currEntry); 
    cmdlineFile = fopen(cmdPath, "rb");

    currNewProc->command = malloc(sizeof(char*) * 512);
    int ns = fscanf(cmdlineFile, "%s", cmdlineBuf);
    for (int i = 0; i < ns; i++) {
        if (!cmdlineBuf[i] && cmdlineBuf[i] == '\0') cmdlineBuf[i] = ' '; 
    }
    strcpy(currNewProc->command, cmdlineBuf);
    fclose(cmdlineFile);
}

char* get_user_from_euid(uid_t euid) {
    struct passwd *pwd = getpwuid(euid);
    if (pwd == NULL) {
        perror("getpwuid");
        return NULL;
    }
    return pwd->pw_name;
}

void proc_cpu_usage(Proc currProc) {
    char uptimeBuf[128];
    char* token;
    long sysUptime, proc_uTimeSec, proc_sTimeSec, proc_startSec;
    FILE* sysUptimeFile;
    sysUptimeFile = fopen("/proc/uptime", "r");

    fgets(uptimeBuf, 128, sysUptimeFile);
    token = strtok(uptimeBuf, " ");
    sysUptime = strtol(token, NULL, 0);

    proc_uTimeSec = currProc->proc_utime / 100;
    proc_sTimeSec = currProc->proc_stime / 100;
    proc_startSec = currProc->proc_startTime / 100;

    currProc->proc_elapsedSec = sysUptime - proc_startSec;
    currProc->proc_usgInSec = proc_uTimeSec + proc_sTimeSec;

    fclose(sysUptimeFile);
}

char* strformat_ms_time(long totalSeconds) {
    char* hmsTime = malloc(sizeof(char) * 256);
    // char strMinutes[16], strSeconds[16];

    int minutes = (int) (totalSeconds / 60) % 60;
    int seconds = (int) totalSeconds % 60;
    // sprintf(strMinutes, "%d", minutes);
    // sprintf(strSeconds, "%d", seconds);
    snprintf(hmsTime, sizeof(char) * 256, "%d:%d", minutes, seconds);
    return hmsTime;
}

int is_pid_dir(const struct dirent *entry) {
    const char *p;
    for (p = entry->d_name; *p; p++) {
        if (!isdigit(*p)) return 0;
    }
    return 1;
}

int pid_exist(Array procArray, const struct dirent *entry) {
    Proc processes = (Proc) procArray->array;
    for (int i = 0; i < procArray->size; i++) {
        if (processes[i].PID == atoi(entry->d_name)) return i;
    }
    return -1;
}