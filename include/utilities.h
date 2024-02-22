#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct node* Node;
typedef struct list* List;
typedef struct array* Array;

struct node {
    void* data;
    Node next;
};

struct list {
    int size;
    Node head;
    Node tail;
};


struct array {
    int size;
    size_t capacity;
    void* array;
};


// Linked list definitions
List new_list();

void add_into(List list, void* data);

void* get_from(List list, int nNode);

void destroy(List list);

// Dynamic array definitions
void init_array(Array self, size_t selfCapacity);

void push_back(Array self, void* newData);

void remove_from_array(Array self, int index);



#endif