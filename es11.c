#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define DELAY 1000

typedef enum {false, true} Boolean;

struct gestore_t {
    sem_t semR, mutex;

    sem_t semProduttori[N];
    Boolean blocked[N];
} gestore;


void initManager(struct gestore_t *g){
    sem_init(&g->semR, 0, 1);
    sem_init(&g->mutex, 0, 1);

    for(int cnt=0; cnt<N; cnt++){
        sem_init(&g->semProduttori[cnt], 0, 0);
        g->blocked[cnt] = false;
    }
}


void pausetta(void) {
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % DELAY + 1) * 1000000;
    nanosleep(&t, NULL);
}

void dispR(struct gestore_t *g, int threadId){
    printf("Sono il thread P%d, sto aspettando il mutex\n", threadId);
    sem_wait(&g->mutex);
    printf("Sono il thread P%d, R è di nuovo disponibile\n", threadId);
    sem_post(&g->semR);
    sem_post(&g->mutex);
}


void segnalaA(struct gestore_t *g, int threadId){
    int v;

    sem_wait(&g->mutex);

    sem_getvalue(&g->semR, &v);
    
    if(v){
        printf("Sono il thread P%d, R è disponibile quindi mi sospendo\n", threadId);
        g->blocked[threadId] = true;
        sem_post(&g->mutex);
        sem_wait(&g->semProduttori[threadId]);

        printf("Sono il thread P%d e sono ripartito\n", threadId);

        dispR(g, threadId);
    }
    else{    
        int cnt=0;
        for(cnt=0; cnt<=threadId; cnt++){
            
            if(g->blocked[cnt]){
                printf("Sono il thread P%d e sto sbloccando il P%d\n", threadId, cnt);
                g->blocked[cnt] = false;
                sem_post(&g->mutex);
                sem_post(&g->semProduttori[cnt]);
                

                printf("Sono il thread P%d, mi sospendo\n", threadId);
                sem_wait(&g->semProduttori[threadId]);
                printf("Sono il thread P%dE MI SONO SBLOCCATO\n", threadId);

                dispR(g, cnt);
                break;
            }
        }

        if(cnt==threadId){
            printf("CIAO");
            dispR(g, threadId);
        }
    }
}


void usaR(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);

    sem_wait(&g->semR);
    printf("Sono il thread C%d e sto usando R\n", threadId);
    sleep(1);
    printf("Sono il thread C%d e sto rilasciando R\n", threadId);

    sem_post(&g->mutex);
}


void testaA(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);

    int v;
    sem_getvalue(&g->semR, &v);

    if(v){
        sem_post(&g->mutex);

        usaR(g, threadId);
    }
    else{
        printf("Sono il thread C%d, R è occupata e mi sospendo\n", threadId);
        sem_post(&g->mutex);

        sem_wait(&g->semR);

        usaR(g, threadId);
    }
    
}



void *execProduttori(void *arg){
    for(;;){
        pausetta();
        segnalaA(&gestore, (int) arg);
    }
}


void *execConsumatori(void *arg){
    for(;;){
        testaA(&gestore, (int) arg);
        pausetta();
    }
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


    for(i = 0; i < N; i++){
        pthread_create(&thread, &threadAttr, execConsumatori, (void *) i);
        pthread_create(&thread, &threadAttr, execProduttori, (void *) i);
    }

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}