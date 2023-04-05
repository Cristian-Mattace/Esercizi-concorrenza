#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <sched.h>

#define DELAY 1000
#define N 4

typedef enum {false, true} Boolean;

//struttura manager contenente mutex e 2 risorse comuni
struct gestore_t{
    sem_t mutex;

    sem_t R1;
    sem_t R2;
} gestore;

//inizializzazione gestore con semafori
void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);

    sem_init(&g->R1, 0, 1);
    sem_init(&g->R2, 0, 1);
}


//MODIFICATA
void pausetta(int T)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand()%T+1)*1000000;
    nanosleep(&t,NULL);
}

//prende la risorsa 
void getR(struct gestore_t *g, int threadId, int T){
    int semValue;
    Boolean getResource = false;

    printf("Sono il thread %d e voglio una risorsa\n", threadId);

    //ciclo sulla T, se si libera una risorsa mentre aspetto la prendo
    for(int i=0; i<T; i++){

        sem_wait(&g->mutex);
        sem_getvalue(&g->R1, &semValue);
        if(semValue!=0){
            sem_wait(&g->R1);
            sem_post(&g->mutex);

            printf("Sono il thread %d e sto usando R1\n", threadId);
            pausetta(DELAY);
            sem_post(&g->R1);
            printf("Sono il thread %d ed ho rilascito R1\n", threadId);
            getResource = true;
            break;
        }
        else{
            sem_getvalue(&g->R2, &semValue);

            if(semValue!=0){
                sem_wait(&g->R2);
                sem_post(&g->mutex);

                printf("Sono il thread %d e sto usando R2\n", threadId);
                pausetta(DELAY);
                sem_post(&g->R2);
                printf("Sono il thread %d ed ho rilascito R2\n", threadId);
                getResource = true;
                break;
            }
        }

        sem_post(&g->mutex);
    }

    if(!getResource)
        printf("Sono il thread %d, è scaduto il mio T\n", threadId);
}


//funzione base del cliente
void *execThread(void *arg)
{
    int T;
    for(;;){
        T = rand() % 10000000 +1000000;
        getR(&gestore, (int) arg, T);
        pausetta(DELAY);
    }

    return 0; 
}


int main (int argc, char **argv) {
    pthread_attr_t attr;
    pthread_t thread;
    

    /* inizializzo il mio sistema */
    init_gestore(&gestore);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&attr);

    for(int i=0; i<N; i++){
        pthread_create(&thread, &attr, execThread, (void *)i);
    }

    pthread_attr_destroy(&attr);

    for(int i=0; i<N; i++){
        pthread_join(thread, NULL);
    }

    return 0;
}