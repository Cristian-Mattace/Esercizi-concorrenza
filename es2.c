#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NUM_THREADS 3

typedef enum {false, true} Boolean;

//definisco la struttura comune
struct R_t{
  int A;
  int B;
};

struct gestore_t{
    sem_t mutexA, mutexB;
    sem_t semA, semB;

    struct R_t R;

    Boolean countR;
    int nA_blocked, nB_blocked;
} gestore;


void init_gestore(struct gestore_t *g){
    sem_init(&g->mutexA, 0, 1);
    sem_init(&g->mutexB, 0, 1);

    sem_init(&g->semA, 0, 1);
    sem_init(&g->semB, 0, 1);

    g->countR = false;
    g->nA_blocked = 0;
    g->nB_blocked = 0;

    g->R.A = 0;
    g->R.B = 0;
}


//funzione per sequenza 2
void exeProcA(struct gestore_t *g, int threadId){

    for(;;){
        if(g->countR == true){
            g->nA_blocked++;
            sem_wait(&g->semA);
        }
        else{
            sem_wait(&g->mutexA);
            g->R.A++;
            printf("Sono il pthread %d e sto incrementando A, A=%d\n", threadId, g->R.A);
            sem_post(&g->mutexA);   
            //sleep(3);
        }
    }
}

void exeProcB(struct gestore_t *g, int threadId){
    for(;;){
        if(g->countR == true){
            g->nB_blocked++;
            sem_wait(&g->semB);
        }
        else{
            sem_wait(&g->mutexB);
            g->R.B++;
            printf("Sono il pthread %d e sto incrementando B, B=%d\n", threadId, g->R.B);
            sem_post(&g->mutexB);
            //sleep(3);
        }
    }
}

void reset(struct gestore_t *g, int threadId){
    g->countR = true;

    printf("Sono il reset e sto aspettando\n");

    //lock sul mutex
    sem_wait(&g->mutexA);
    sem_wait(&g->mutexB);

    g->R.A = 0;
    g->R.B = 0;

    g->countR = false;

    for(int i=0; i<g->nA_blocked; i++) sem_post(&g->semA);
    for(int i=0; i<g->nB_blocked; i++) sem_post(&g->semB);
    g->nA_blocked = 0;
    g->nB_blocked = 0;

    printf("Sono il reset e sto resettando A e B\n");

    sem_post(&g->mutexA);
    sem_post(&g->mutexB);

    //sleep(7);
}


void *exeReset(void *arg){

    for(;;){
        reset(&gestore, (int) arg);
    }
}

//funzione eseguita dai thread di default
void *exeThread(void *arg) {

    for(;;){
        if((rand() % 10 % 2) == 0){
            exeProcA(&gestore, (int) arg);
        }
        else{
            exeProcB(&gestore, (int) arg);
        }
    }

    // terminates the current thread and returns the integer value of the index
    //pthread_exit((void *) arg);
}



int main (int argc, char **argv) {
    pthread_attr_t a;
    pthread_t p;

    init_gestore(&gestore);

    pthread_attr_init(&a);

    pthread_create(&p, &a, exeThread, (void *) 0);
    pthread_create(&p, &a, exeThread, (void *) 1);
    pthread_create(&p, &a, exeThread, (void *) 2);
    pthread_create(&p, &a, exeThread, (void *) 3);
    pthread_create(&p, &a, exeThread, (void *) 4);
    pthread_create(&p, &a, exeThread, (void *) 5);

    pthread_create(&p, &a, exeReset, (void *) 6);   //RESET!!!

    pthread_attr_destroy(&a);

    for(int i=0; i<7; i++){
        pthread_join(p, NULL);
    }


    return 0;
}