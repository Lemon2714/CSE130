#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LIST
#define LIST

typedef struct ListObj *List;

// List creation and deletion
List newList(void);
void freeList(List *pL);

// List properties
int length(List L);
int curr_index(List L);
void *front(List L);
void *back(List L);
char *getfile(List L);
void *getlock(List L);

// List navigation
void moveFront(List L);
void moveBack(List L);
void movePrev(List L);
void moveNext(List L);

// List modification
void clear(List L);
void set(List L, void *x);
void append(List L, char *filename, void *rwl);
void deleteFront(List L);
void delete (List L);

#endif
