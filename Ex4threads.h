#define _GNU_SOURCE
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
typedef struct Vertice{
char *Name,*IP;
int port;//the real one
int* dv;//[0/1,dv]
}Vertice;

typedef struct Graph{
struct Vertice* v;
int ** Connections;
char** assigns;
int length;
}Graph;

int getnumber(char* arr,char* a,int leng);
Vertice* getverticbychar(struct Vertices* v,char ch,int len);
void BellmanFord(struct Graph * g,char source);
void NewInput(char* loc,struct Graph* g);
int isnotin(char* a,char** arr);
