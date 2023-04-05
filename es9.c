#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define INT_NULL -1
#define DELAY 1000

typedef enum {false, true} Boolean;
typedef int T;

//struct gestore con array di semafori per il buffer, semaforo ricevente, semaforo A e B, buffer, contatore e bool isFirst
struct gestore_t {
    sem_t semR[N];
    sem_t semReceiver;

    sem_t semA, semB;

    T buffer[N];
    int count;

    Boolean isFirst;
} gestore;

//inizializzo il gestore con array semaforo buffer tutto ad 1, semReceiver a 0, sem A e B ad 1, count a 0 e isFirst a true
void initManager(struct gestore_t *g){
    for(int i = 0; i < N; i++){
        sem_init(&g->semR[i], 0 , 1);
        g->buffer[i] = INT_NULL;
    }

    sem_init(&g->semReceiver, 0, 0);

    sem_init(&g->semA, 0, 1);
    sem_init(&g->semB, 0, 1);

    g->count = 0;
    g->isFirst = true;
}


void pausetta(void) {
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % DELAY + 1) * 1000000;
    nanosleep(&t, NULL);
}


//wait su semA, se è il primo mette isFirst a false ed esegue una wait (passante) su semB
//controlla il valore del semaforo buffer sulla sua posizione, così se ha già scritto non può rifarlo
//se può scrivere esegueuna wait sul semaforo buffer nella sua posizione, scrive nel buffer, incrementa il count
//controlla il count, se =5 allora sveglia il receiver, perchè il buffer è pieno
//altrimenti fa una post su semA, così il processo successivo del suo stesso gruppo potrà entrare
void sendA(struct gestore_t *g, int threadId){

    sem_wait(&g->semA);

    if(g->isFirst){
        g->isFirst = false;
        sem_wait(&g->semB);
    }

    int v;
    sem_getvalue(&g->semR[threadId], &v);

    if(v!=0){

        sem_wait(&g->semR[threadId]);

        T msg = rand() % 10000;
        g->buffer[threadId] = msg;

        printf("Sono il thread A%d, ho inserito %d\n", threadId, msg);
        
        g->count++;

        if(g->count == N){
            printf("Sono il thread A%d, sto per svegliare il receiver\n", threadId);
            sem_post(&g->semReceiver);
        }
        else{
            sem_post(&g->semA);
        }
    }
    else{
        sem_post(&g->semA);
    }
}


//stesso funzionamento si sendA, ma con B
void sendB(struct gestore_t *g, int threadId){
    sem_wait(&g->semB);

    if(g->isFirst){
        g->isFirst = false;
        sem_wait(&g->semA);
    }

    int v;
    sem_getvalue(&g->semR[threadId], &v);

    if(v!=0){

        sem_wait(&g->semR[threadId]);

        T msg = rand() % 10000;
        g->buffer[threadId] = msg;

        printf("Sono il thread B%d, ho inserito %d\n", threadId, msg);
        
        g->count++;


        if(g->count == N){
            printf("Sono il thread B%d, sto per svegliare il receiver\n", threadId);
            sem_post(&g->semReceiver);
        }
        else{
            sem_post(&g->semB);
        }
    }
    else{
        sem_post(&g->semB);
    }
}


//il receiver inizialmente ha il semaforo a 0, quindi la prima wait è bloccante
//quando il buffer è pieno, un thread esegue la post sul semaforo ricevente, così sblocca il processo
//il quale legge il buffer e lo scrive in output
//infine imposta il count=0, isFirst a true ed esegue una post su semA e semB, oltre che alla post sull'array semaforo del buffer
void receive(struct gestore_t *g){
    sem_wait(&g->semReceiver);

    printf("Sono il receiver, il buffer è:");
    for(int i = 0; i < N; i++){
        printf("%d", g->buffer[i]);
        if(i != N - 1)
            printf(", ");
        else
            printf("\n");

        g->buffer[i] = INT_NULL;
    }

    g->count = 0;
    g->isFirst = true;
    sem_post(&g->semA);
    sem_post(&g->semB);

    for(int i = 0; i < N; i++)
        sem_post(&g->semR[i]);
}

void *senderA(void *arg){
    for(;;){
        pausetta();
        sendA(&gestore, (int)arg);
    }
    return 0;
}

void *senderB(void *arg){
    for(;;){
        pausetta();
        sendB(&gestore, (int)arg);
    }
    return 0;
}

void *receiver(void *arg){
    for(;;){
        receive(&gestore);
    }
    return 0;
}

int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    initManager(&gestore);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, senderA, (void *) i);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, senderB, (void *) i);

    pthread_create(&thread, &threadAttr, receiver, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N*2 + 1; i++)
        pthread_join(thread, NULL);

    return 0;
}