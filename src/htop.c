#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include <sys/sysinfo.h>
#include <signal.h>

#include "../include/htop.h"
#include "../include/processes_stat.h"
#include "../include/utilities.h"



#define MAX_CORES 32
#define GAUGE_WIDTH 22
#define MY_HTOP
#ifdef MY_HTOP


Array processesArray = NULL;
Proc processes;
int num_processes;
int scroll_offset = 0;
int selected_index = 0;
int sort_property = 0;

enum {
    COLOR_DEFAULT = 1,
    COLOR_SELECTED = 2
};



void mem_usage() {
    struct sysinfo info;
    sysinfo(&info);

    printw("    Total RAM: %lu MB\t\t\t\t\t\t\tNumber of Processes: %d\n", info.totalram / (1024 * 1024), info.procs);
    printw("    Free RAM: %lu MB\t\t\t\t\t\t\tNumber of Threads: %d\n", info.freeram / (1024 * 1024), info.procs);
    // printw("    Number of Processes: %d\n", info.procs);
    // printw("    Number of Threads: %d\n", info.procs);
    printw("    \t\t\t\t\t\t\t\t\tLoad Average: %.2f, %.2f, %.2f\n", info.loads[0] / 65536.0, info.loads[1] / 65536.0, info.loads[2] / 65536.0);
    printw("\n");
    refresh();
}

// void sort_processes() {
//     qsort(processes, num_processes, sizeof(Proc), compare_proc);
// }

// int compare_proc(const void* a, const void* b) {
//     Proc* proc1 = (Proc*)a;
//     Proc* proc2 = (Proc*)b;

//     switch (sort_property) {
//         case 0: // Sort by PID
//             return proc1->PID - proc2->PID;
//         case 1: // Sort by priority
//             return proc1->priority - proc2->priority;
//         case 2: // Sort by niceness
//             return proc1->niceness - proc2->niceness;
//         case 3: // Sort by virtual memory usage
//             return proc1->virt_usg - proc2->virt_usg;
//         case 4: // Sort by resident memory usage
//             return proc1->res_usg - proc2->res_usg;
//         case 5: // Sort by shared memory usage
//             return proc1->shr_usg - proc2->shr_usg;
//         // Add more cases for additional sorting options as needed
//         default:
//             return 0; // Default to no sorting
//     }
// }

// void handle_sort_input(int ch) {
//     switch (ch) {
//         case '1':
//             sort_property = 0; // Sort by PID
//             break;
//         case '2':
//             sort_property = 1; // Sort by priority
//             break;
//         case '3':
//             sort_property = 2; // Sort by niceness
//             break;
//         case '4':
//             sort_property = 3; // Sort by virtual memory usage
//             break;
//         case '5':
//             sort_property = 4; // Sort by resident memory usage
//             break;
//         case '6':
//             sort_property = 5; // Sort by shared memory usage
//             break;
//         // Add more cases for additional sorting options as needed
//     }
// }

void processes_stats() {
    // Assuming you have a function named "retrieve_process_stats" that fills the dynamic array "processes"
    // This function retrieves process stats and updates the global "num_processes" variable
    if (!processesArray) {
        processesArray = malloc(sizeof(struct array));
    }
    running_processes(processesArray);
    processes = (Proc) processesArray->array;
    num_processes = processesArray->size;
    // sort_processes();
}

// Function to print the header with CPU usage gauges using ncurses
void print_header(int num_cores, double* cpu_usage) {
    clear();
    printw("\n");
    
    int num_rows = num_cores / 4 + (num_cores % 4 != 0); // Calculate number of rows for gauges
    for (int i = 0; i < num_rows; ++i) {
        printw("    ");
        for (int j = 0; j < 4 && i * 4 + j < num_cores; ++j) {
            int core_id = i * 4 + j;
            if (core_id < 10) printw(" %d[", core_id); 
            else printw("%d[", core_id);
            // Print the gauge based on CPU usage percentage
            for (int k = 0; k < GAUGE_WIDTH; ++k) {
                if (k < cpu_usage[core_id] / (100 / GAUGE_WIDTH)) {
                    printw("|");
                } else {
                    printw(" ");
                }
            }
            printw("%.1f%%]\t", cpu_usage[core_id]);
        }
        printw("\n");
    }
    mem_usage();
}

void print_process_list() {
    // clear();
    // print_system_info();
    int max_y = getmaxy(stdscr);

    attron(COLOR_PAIR(COLOR_SELECTED));
    printw("    %-5s %-10s %-3s %-3s %-7s %-7s %-7s %-2s %-5s %-5s %-s %*s\n",
           "PID", "USER", "PRI", "NI", "VIRT", "RES", "SHR", "S", "CPU", "TIME", "COMMAND", 63, "");
    attroff(COLOR_PAIR(COLOR_SELECTED));
    
    int display_count = 0;
    for (int i = scroll_offset; i < num_processes && display_count < max_y - 6; ++i) {
        char* procTimeFormatted = strformat_ms_time(processes[i].proc_usgInSec);
        if (i == selected_index) {
            attron(A_REVERSE);
        }
        printw("    %-5d %-10s %-3d %-3d %-7ld %-7ld %-7ld %-2c %-5.1f %-5s %-s\n",
            processes[i].PID, processes[i].processUser, processes[i].priority,
            processes[i].niceness, processes[i].virt_usg, processes[i].res_usg,
            processes[i].shr_usg, processes[i].state, (float)(processes[i].proc_usgInSec * 100) / processes[i].proc_elapsedSec,
            procTimeFormatted, processes[i].command);
    // if (i == selected_index)
        attroff(A_REVERSE);
        display_count++;
    }
    refresh();
}

void print_input_legend() {
    int max_y, max_x, iCol = 0, commandIndex = 0;
    char inputList[] = {'q', 'k'};
    char* commandList[] = {"QUIT", "KILL"};
    int nCommand = sizeof(commandList) / sizeof(commandList[0]);
    getmaxyx(stdscr, max_y, max_x);

    // Print the legend at the bottom of the screen
    while (iCol < max_x && commandIndex < nCommand) {
        mvprintw(max_y - 1, iCol+1, "%c", inputList[commandIndex]);
        attron(COLOR_PAIR(COLOR_SELECTED));
        mvprintw(max_y - 1, iCol+2, "%s", commandList[commandIndex]);
        attroff(COLOR_PAIR(COLOR_SELECTED));
        commandIndex++;
        iCol += 5;
    }
    attron(COLOR_PAIR(COLOR_SELECTED));
    mvprintw(max_y - 1, iCol+1, "%*s", max_x - iCol, "");
    attroff(COLOR_PAIR(COLOR_SELECTED));
    // mvprintw(max_y - 1, 0, "Press arrow keys to navigate, 'k' to kill process");
    // mvprintw(max_y - 1, max_x - 30, "Press 1-6 to sort by property");
}

void handle_input() {
    int ch = getch();
    int max_y = getmaxy(stdscr);
    switch (ch) {
        case KEY_UP:
            if (selected_index > 0) {
                selected_index--;
                if (selected_index < scroll_offset)
                    scroll_offset--;
            } else if (selected_index == 0 && scroll_offset > 0) {
                // If at the top and scroll_offset is greater than 0, scroll up
                scroll_offset--;
            }
            break;
        case KEY_DOWN:
            if (selected_index < num_processes - 1) {
                selected_index++;
                if (selected_index >= scroll_offset + max_y - 1)
                    scroll_offset++;
            } else if (selected_index == num_processes - 1) {
                // If at the bottom, scroll down
                scroll_offset++;
                refresh();
            }
            break;
        case 'k':
            if (selected_index >= 0 && selected_index < num_processes) {
                kill(processes[selected_index].PID, SIGKILL);
                processes_stats(); // Refresh the process list after killing
                // print_process_list();
            }
            break;
        case 'q':
            endwin();
            exit(0);
            break;
        // default:
        //     handle_sort_input(ch);
        //     break;
    }
}


int main() {
    initscr(); // Initialize ncurses
    start_color(); // Enable color
    init_pair(COLOR_DEFAULT, COLOR_WHITE, COLOR_BLACK); // Define color pair for default text
    init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_YELLOW);
    curs_set(0); // Hide cursor
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable keypad

    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    double* cpu_usage = malloc(sizeof(double) * MAX_CORES);

    while (1) {
        processes_stats();
        sys_cpu_usage(cpu_usage, num_cores);
        clear();
        print_header(num_cores, cpu_usage);
        print_process_list();
        print_input_legend();
        handle_input();
        usleep(10000); // Update every 0.1 second
    }

    endwin(); // End ncurses

    return 0;
}