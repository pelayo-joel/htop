#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

#include "../include/htop.h"
#include "../include/processes_stat.h"
#include "../include/utilities.h"


#define DEBUG_PROCESS
// #define GPT_HTOP_NOSCROLL
// #define GPT_HTOP_SCROLL
// #define GPT_HTOP_SCROLLANDHEADER
#define MY_HTOP



#ifdef MY_HTOP
int main() {
    Array processes = running_processes();
    Proc allProcesses = (Proc) processes->array;
    
    #ifdef DEBUG_PROCESS
    printf("PID\tPriority\tNiceness\tVIRT\tRES\tSHR\tutime\tstime\tstart\tState\tCommand\n");
    #endif
    for (int i = 0; i < processes->size; i++) {
        #ifdef DEBUG_PROCESS
        printf("%d\t%d\t        %d\t        %ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%c\t%s\n", 
            allProcesses->PID, allProcesses->priority, allProcesses->niceness,
            allProcesses->virt_usg, allProcesses->res_usg, allProcesses->shr_usg,
            allProcesses->proc_utime, allProcesses->proc_stime, allProcesses->proc_startTime,
            allProcesses->state, allProcesses->command);
        #endif
        allProcesses++;
    }

    // initscr();
    // start_color();
    // init_pair(1, COLOR_RED, COLOR_BLACK);
    // mvprintw(1, 4, "%c", '[');
    // attron(COLOR_PAIR(1));
    // for (int i = 6; i < 52; i++) {
    //     mvprintw(1, i, "%c", '|');
    // }
    // attroff(COLOR_PAIR(1));
    // mvprintw(1, 53, "%c", ']');
    // refresh();
    // getch();
    // endwin();
    return 0;
}
#endif





#define MAX_PROCESSES 1000
#define MAX_PROCESS_NAME_LEN 256

#ifdef GPT_HTOP_NOSCROLL
typedef struct {
    char name[MAX_PROCESS_NAME_LEN];
    int pid;
    float cpu_usage;
    float mem_usage;
} ProcessInfo;

ProcessInfo processes[MAX_PROCESSES];
int num_processes = 0;
int scroll_offset = 0;
int selected_index = 0;

void get_process_info() {
    FILE *fp = popen("ps aux", "r");
    if (fp == NULL) {
        perror("Error running ps aux");
        exit(1);
    }

    // Ignore the first line (header)
    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp);

    num_processes = 0;
    while (fgets(buffer, sizeof(buffer), fp) && num_processes < MAX_PROCESSES) {
        char name[MAX_PROCESS_NAME_LEN];
        int pid;
        float cpu_usage, mem_usage;

        sscanf(buffer, "%*s %d %*s %*s %*s %*s %*s %*s %*s %f %f %*s %s", &pid, &cpu_usage, &mem_usage, name);

        strncpy(processes[num_processes].name, name, MAX_PROCESS_NAME_LEN);
        processes[num_processes].pid = pid;
        processes[num_processes].cpu_usage = cpu_usage;
        processes[num_processes].mem_usage = mem_usage;

        num_processes++;
    }

    pclose(fp);
}

void print_process_list() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    printw("%-10s %-10s %-10s %-s\n", "PID", "CPU%", "MEM%", "COMMAND");
    int display_count = 0;
    for (int i = scroll_offset; i < num_processes && display_count < max_y - 1; ++i) {
        if (i == selected_index)
            attron(A_REVERSE);
        printw("%-10d %-10.2f %-10.2f %-s\n", processes[i].pid, processes[i].cpu_usage, processes[i].mem_usage, processes[i].name);
        if (i == selected_index)
            attroff(A_REVERSE);
        display_count++;
    }
    refresh();
}

void handle_input() {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            if (selected_index > 0) {
                selected_index--;
                if (selected_index < scroll_offset)
                    scroll_offset--;
            }
            break;
        case KEY_DOWN:
            if (selected_index < num_processes - 1) {
                selected_index++;
                int max_y, max_x;
                getmaxyx(stdscr, max_y, max_x);
                if (selected_index >= scroll_offset + max_y - 1)
                    scroll_offset++;
            }
            break;
        case 'q':
            endwin();
            exit(0);
            break;
    }
}

int main() {
    initscr(); // Initialize ncurses
    curs_set(0); // Hide cursor
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable keypad

    while (1) {
        get_process_info();
        print_process_list();
        handle_input();
        usleep(5000); // Update every half second
    }

    endwin(); // End ncurses
    return 0;
}
#endif





#ifdef GPT_HTOP_SCROLL
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 1000
#define MAX_COMMAND_LEN 256

typedef struct processe {
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
} Process;

Process processes[MAX_PROCESSES];
int num_processes = 0;
int scroll_offset = 0;
int selected_index = 0;

void get_process_info() {
    FILE *fp = popen("ps -e -o pid,pri,ni,user,vsz,rss,pmem,utime,stime,start,state,cmd --sort=-pcpu", "r");
    if (fp == NULL) {
        perror("Error running ps");
        exit(1);
    }

    // Ignore the first line (header)
    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp);

    num_processes = 0;
    while (fgets(buffer, sizeof(buffer), fp) && num_processes < MAX_PROCESSES) {
        Process proc;
        proc.command = malloc(MAX_COMMAND_LEN);
        sscanf(buffer, "%d %d %d %s %ld %ld %ld %ld %ld %ld %c %s",
               &proc.PID, &proc.priority, &proc.niceness, proc.processUser,
               &proc.virt_usg, &proc.res_usg, &proc.shr_usg, &proc.proc_utime,
               &proc.proc_stime, &proc.proc_startTime, &proc.state, proc.command);

        processes[num_processes++] = proc;
    }

    pclose(fp);
}

void print_process_list() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    printw("%-10s %-5s %-5s %-15s %-10s %-10s %-10s %-10s %-10s %-10s %-5s %-s\n",
           "PID", "PRI", "NI", "USER", "VIRT", "RES", "SHR", "UTIME", "STIME", "START", "S", "COMMAND");
    int display_count = 0;
    for (int i = scroll_offset; i < num_processes && display_count < max_y - 1; ++i) {
        if (i == selected_index)
            attron(A_REVERSE);
        printw("%-10d %-5d %-5d %-15s %-10ld %-10ld %-10ld %-10ld %-10ld %-10ld %-5c %-s\n",
               processes[i].PID, processes[i].priority, processes[i].niceness,
               processes[i].processUser, processes[i].virt_usg, processes[i].res_usg,
               processes[i].shr_usg, processes[i].proc_utime, processes[i].proc_stime,
               processes[i].proc_startTime, processes[i].state, processes[i].command);
        if (i == selected_index)
            attroff(A_REVERSE);
        display_count++;
    }
    refresh();
}

void handle_input() {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            if (selected_index > 0) {
                selected_index--;
                if (selected_index < scroll_offset)
                    scroll_offset--;
            }
            break;
        case KEY_DOWN:
            if (selected_index < num_processes - 1) {
                selected_index++;
                int max_y, max_x;
                getmaxyx(stdscr, max_y, max_x);
                if (selected_index >= scroll_offset + max_y - 1)
                    scroll_offset++;
            }
            break;
        case 'q':
            endwin();
            exit(0);
            break;
    }
}

int main() {
    initscr(); // Initialize ncurses
    // curs_set(0); // Hide cursor
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable keypad

    while (1) {
        get_process_info();
        print_process_list();
        handle_input();
        usleep(5000); // Update every half second
    }

    endwin(); // End ncurses
    return 0;
}
#endif





#ifdef GPT_HTOP_SCROLLANDHEADER
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h> // For getting system information

#define MAX_PROCESSES 1000
#define MAX_COMMAND_LEN 256

typedef struct processus {
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
} Process;

Process processes[MAX_PROCESSES];
int num_processes = 0;
int scroll_offset = 0;
int selected_index = 0;

void get_process_info() {
    FILE *fp = popen("ps -e -o pid,pri,ni,user,vsz,rss,pmem,utime,stime,start,state,cmd --sort=-pcpu", "r");
    if (fp == NULL) {
        perror("Error running ps");
        exit(1);
    }

    // Ignore the first line (header)
    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp);

    num_processes = 0;
    while (fgets(buffer, sizeof(buffer), fp) && num_processes < MAX_PROCESSES) {
        Process proc;
        proc.command = malloc(MAX_COMMAND_LEN);
        sscanf(buffer, "%d %d %d %s %ld %ld %ld %ld %ld %ld %c %s",
               &proc.PID, &proc.priority, &proc.niceness, proc.processUser,
               &proc.virt_usg, &proc.res_usg, &proc.shr_usg, &proc.proc_utime,
               &proc.proc_stime, &proc.proc_startTime, &proc.state, proc.command);

        processes[num_processes++] = proc;
    }

    pclose(fp);
}

void print_machine_info() {
    struct sysinfo info;
    sysinfo(&info);

    printw("System Info:\n");
    printw("  Load Average: %.2f, %.2f, %.2f\n", info.loads[0] / 65536.0, info.loads[1] / 65536.0, info.loads[2] / 65536.0);
    printw("  Total RAM: %lu MB\n", info.totalram / (1024 * 1024));
    printw("  Free RAM: %lu MB\n", info.freeram / (1024 * 1024));
    printw("  Number of Processes: %d\n", info.procs);
    printw("  Number of Threads: %d\n", info.procs);
    printw("\n");
}

void print_process_list() {
    clear();
    print_machine_info();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    printw("%-10s %-5s %-5s %-15s %-10s %-10s %-10s %-10s %-10s %-10s %-5s %-s\n",
           "PID", "PRI", "NI", "USER", "VIRT", "RES", "SHR", "UTIME", "STIME", "START", "S", "COMMAND");
    int display_count = 0;
    for (int i = scroll_offset; i < num_processes && display_count < max_y - 6; ++i) {
        if (i == selected_index)
            attron(A_REVERSE);
        printw("%-10d %-5d %-5d %-15s %-10ld %-10ld %-10ld %-10ld %-10ld %-10ld %-5c %-s\n",
               processes[i].PID, processes[i].priority, processes[i].niceness,
               processes[i].processUser, processes[i].virt_usg, processes[i].res_usg,
               processes[i].shr_usg, processes[i].proc_utime, processes[i].proc_stime,
               processes[i].proc_startTime, processes[i].state, processes[i].command);
        if (i == selected_index)
            attroff(A_REVERSE);
        display_count++;
    }
    refresh();
}

void handle_input() {
    int ch = getch();
    switch (ch) {
        case KEY_UP:
            if (selected_index > 0) {
                selected_index--;
                if (selected_index < scroll_offset)
                    scroll_offset--;
            }
            break;
        case KEY_DOWN:
            if (selected_index < num_processes - 1) {
                selected_index++;
                int max_y, max_x;
                getmaxyx(stdscr, max_y, max_x);
                if (selected_index >= scroll_offset + max_y - 6)
                    scroll_offset++;
            }
            break;
        case 'q':
            endwin();
            exit(0);
            break;
    }
}

int main() {
    initscr(); // Initialize ncurses
    curs_set(0); // Hide cursor
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable keypad

    while (1) {
        get_process_info();
        print_process_list();
        handle_input();
        usleep(5000); // Update every half second
    }

    endwin(); // End ncurses
    return 0;
}

#endif