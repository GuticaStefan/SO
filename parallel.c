#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
os_graph_t *graph;

// structura folosita pentru a fi trimisa ca parametru functiei processNode
// la adaugarea unui nou task in queue
typedef struct{
    os_graph_t *graph;
    os_threadpool_t *threadpool;
    unsigned int nodeID;
}args_t;

pthread_mutex_t sum_lock, visited_lock;

int isEmpty(os_threadpool_t *tp){
    // functia verifica daca mai sunt task-uri de executat si
    // returneaza 1 in cazul in care queue-ul de task-uri este vid
    // sau 0 in cazul in care mai sunt task-uri in queue
    pthread_mutex_lock(&tp->taskLock);
    if(tp->tasks == NULL){
        pthread_mutex_unlock(&tp->taskLock);
        return 1;
    }
    pthread_mutex_unlock(&tp->taskLock);
    return 0;
}

void processNode(void *args){
    /* 
        Daca nodul primit ca parametru din structura a fost
    deja vizitat, opresc executarea functiei. In cazul in care
    este prima data cand il vizitez, il adaug la suma si
    ii parcurg toti vecinii si ii adaug in coada de task-uri
    pe cei nevizitati(bfs).
    */ 
    args_t *arg = args;
    os_graph_t *graph = arg->graph;
    os_threadpool_t *tp = arg->threadpool;
    unsigned int nodeID = arg->nodeID;

    pthread_mutex_lock(&visited_lock);
    if(graph->visited[nodeID] == 1){
        free(args);
        pthread_mutex_unlock(&visited_lock);
        return;
    }
    
    graph->visited[nodeID] = 1;
    pthread_mutex_unlock(&visited_lock);
    pthread_mutex_lock(&sum_lock);
    sum += graph->nodes[nodeID]->nodeInfo;
    pthread_mutex_unlock(&sum_lock);

    for(int i = 0; i < graph->nodes[nodeID]->cNeighbours; i++){
        pthread_mutex_lock(&visited_lock);
        if(graph->visited[graph->nodes[nodeID]->neighbours[i]] == 0){
            pthread_mutex_unlock(&visited_lock);

            args_t *new_arg = (args_t*)calloc(1, sizeof(args_t));
            new_arg->threadpool = tp;
            new_arg->graph = graph;
            new_arg->nodeID = graph->nodes[nodeID]->neighbours[i];
            
            add_task_in_queue(tp, task_create(new_arg, processNode));
        } else {
             pthread_mutex_unlock(&visited_lock);    
        }
         
    }
    free(args);
}

void traverse_graph(os_threadpool_t *tp){

    for(int i = 0; i < graph->nCount; i++){
        pthread_mutex_lock(&visited_lock);
        if(graph->visited[graph->nodes[i]->nodeID] == 0){
            //graph->visited[graph->nodes[i]->nodeID] = 1;
            pthread_mutex_unlock(&visited_lock);

            args_t *args = (args_t*)calloc(1, sizeof(args_t));            
            args->graph = graph;
            args->nodeID = graph->nodes[i]->nodeID;
            args->threadpool = tp;
            
            add_task_in_queue(tp, task_create(args, processNode));
        } else{
            pthread_mutex_unlock(&visited_lock);
        }
        // dupa adaugarea in queue a primului nod astept sa se parcurga
        // toata componenta conexa din care face acesta parte si ulterior
        // repornesc parcurgerea bfs din urmatorul nod nevizitat
        while(!isEmpty(tp)){
            continue;
        }
    }
}

int main(int argc, char *argv[])
{
    
    if (argc != 2)
    {
        printf("Usage: ./main input_file\n");
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    // TODO: create thread pool and traverse the graf
    pthread_mutex_init(&sum_lock, NULL);
    pthread_mutex_init(&visited_lock, NULL);
    os_threadpool_t *tp = threadpool_create(graph->nCount, MAX_THREAD);
    
    traverse_graph(tp);
    
    threadpool_stop(tp, isEmpty);
    
    printf("%d", sum);
    pthread_mutex_destroy(&sum_lock);
    pthread_mutex_destroy(&visited_lock);
    pthread_mutex_destroy(&tp->taskLock);
    free(tp);
    return 0;
}
