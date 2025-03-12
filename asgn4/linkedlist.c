#include "linkedlist.h"
#include <pthread.h>

typedef struct Node {
    char file[69];
    pthread_rwlock_t *rwlock;
    struct Node *next;
    struct Node *prev;
} Node;

struct ListObj {
    Node *front;
    Node *back;
    Node *cursor;
    int length;
    int index;
};

Node *create_node(const char *file, pthread_rwlock_t *rwl) {
    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    strncpy(node->file, file, sizeof(node->file) - 1);
    node->file[sizeof(node->file) - 1] = '\0';
    node->rwlock = rwl;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void free_node(Node *node) {
    if (node) {
        free(node->rwlock);
        free(node);
    }
}

List newList(void) {
    List list = calloc(1, sizeof(struct ListObj));
    if (list == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    list->front = list->back = list->cursor = NULL;
    list->length = 0;
    list->index = -1;
    return list;
}

void freeList(List *pL) {
    if (pL && *pL) {
        clear(*pL);
        free(*pL);
        *pL = NULL;
    }
}

int length(List L) {
    return L ? L->length : -1;
}

int curr_index(List L) {
    return L ? L->index : -1;
}

void *front(List L) {
    return (L && L->front) ? L->front->file : NULL;
}

void *back(List L) {
    return (L && L->back) ? L->back->file : NULL;
}

char *getfile(List L) {
    return (L && L->cursor) ? L->cursor->file : NULL;
}

void *getlock(List L) {
    return (L && L->cursor) ? L->cursor->rwlock : NULL;
}

void clear(List L) {
    while (L && L->length > 0) {
        deleteFront(L);
    }
    if (L) {
        L->cursor = NULL;
        L->index = -1;
    }
}

void moveFront(List L) {
    if (L && L->length > 0) {
        L->cursor = L->front;
        L->index = 0;
    }
}

void moveBack(List L) {
    if (L && L->length > 0) {
        L->cursor = L->back;
        L->index = L->length - 1;
    }
}

void movePrev(List L) {
    if (L && L->cursor && L->cursor->prev) {
        L->cursor = L->cursor->prev;
        L->index--;
    }
}

void moveNext(List L) {
    if (L && L->cursor && L->cursor->next) {
        L->cursor = L->cursor->next;
        L->index++;
    } else if (L && L->cursor) {
        L->cursor = NULL;
        L->index = -1;
    }
}

void append(List L, char *filename, void *rwl) {
    if (L) {
        Node *node = create_node(filename, rwl);
        if (!node)
            return;
        if (L->length == 0) {
            L->front = L->back = node;
        } else {
            node->prev = L->back;
            L->back->next = node;
            L->back = node;
        }
        L->length++;
    }
}

void deleteFront(List L) {
    if (L && L->length > 0) {
        Node *oldFront = L->front;
        if (L->front == L->back) {
            L->front = L->back = NULL;
        } else {
            L->front = L->front->next;
            L->front->prev = NULL;
        }
        free_node(oldFront);
        L->length--;
        if (L->cursor == oldFront) {
            L->cursor = NULL;
            L->index = -1;
        }
    }
}
