#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
    // TODO
    // alocam memorie pentru un nou task si il initializam
    os_task_t *task = (os_task_t*)calloc(1, sizeof(os_task_t));
    if(task == NULL){
        return NULL;
    }
    task->argument = arg;
    task->task = f;
    return task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
    // TODO
    os_task_queue_t *tasks = NULL;
    pthread_mutex_lock(&tp->taskLock);
    if(tp == NULL || t == NULL){
        pthread_mutex_unlock(&tp->taskLock);
        return;
    }
    tasks = tp->tasks;
    // adaugam task-ul in queue pe prima pozitie
    if(tasks == NULL){
        tasks = (os_task_queue_t*)calloc(1, sizeof(os_task_queue_t));
        tasks->task = t;
        tp->tasks = tasks;
    } else {
        os_task_queue_t *node = (os_task_queue_t*)calloc(1, sizeof(os_task_queue_t));
        node->next = tasks;
        tp->tasks = node;
        tp->tasks->task = t;

    }
    pthread_mutex_unlock(&tp->taskLock);
}


/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
    // TODO
    // returnam primul task din queue si modificam pointerul catre
    // queue cu urmatorul task din queue
    os_task_t *temp = NULL;
    pthread_mutex_lock(&tp->taskLock);
    if(tp->tasks){
        temp = tp->tasks->task;
        os_task_queue_t *aux = tp->tasks;
        tp->tasks = tp->tasks->next;
        free(aux);
    }
    pthread_mutex_unlock(&tp->taskLock);
    return temp;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
    // TODO
    // cream un nou threadpool si il initializam
    os_threadpool_t *threadpool = (os_threadpool_t*)calloc(1, sizeof(os_threadpool_t));
    pthread_mutex_init(&threadpool->taskLock, NULL);
    threadpool->num_threads = nThreads;
    threadpool->should_stop = 0;
    // cream nThreads ce executa functia thread_loop_function in care
    // asteapta primirea de noi task-uri ca sa le execute
    threadpool->threads = (pthread_t*)calloc(nThreads, sizeof(pthread_t));
    for(int i = 0; i < threadpool->num_threads; i++){
        pthread_create(&threadpool->threads[i], NULL, thread_loop_function, threadpool);
    }
    return threadpool;
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
   
    // TODO
    // functia primeste ca parametru threadpool-ul si fiecare thread
    // asteapta sa primeasca task-uri ca sa le execute
    os_threadpool_t *tp = (os_threadpool_t*)args; 
    while(1){
        os_task_t *task = get_task(tp);
        if(task){
            task->task(task->argument);
            free(task);
        }

        if(tp->should_stop == 1){
            break;
        }
    }
    pthread_exit(NULL);
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
    // TODO
    // daca se indeplineste conditia de oprire si se apeleaza functia
    // se face join pe toate thread-urile pornite la crearea threadpool-ului
    if(processingIsDone(tp)){
        void *status;
        tp->should_stop = 1;
        for(int i = 0; i < tp->num_threads; i++){
            pthread_join(tp->threads[i], &status);
        }
        free(tp->threads);
    }
}
