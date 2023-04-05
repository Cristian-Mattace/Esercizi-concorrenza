#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 40
#define DELAY 1000
#define NA -1

#define RQ 0
#define RA 1
#define RB 2
#define R2 3

typedef enum {false, true} Boolean;

//struttura priorità con semaforo e contatore di processi in attesa
struct private_sem_t{
    sem_t s;
    int count;
};

struct gestore_t {
    sem_t semA, semB, mutex;
    int request;

    struct private_sem_t semPriority[4];
} gestore;


void initManager(struct gestore_t *g){
    sem_init(&g->semA, 0, 1);
    sem_init(&g->semB, 0, 1);
    sem_init(&g->mutex, 0, 1);

    g->request = NA;

    for(int cnt=0; cnt<4; cnt++){
        sem_init(&g->semPriority[cnt].s, 0, 0);
        g->semPriority[cnt].count = 0;
    }
}


void pausetta(void) {
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % DELAY + 1) * 1000000;
    nanosleep(&t, NULL);
}

//richiedo la risorsa e mi metto in attesa sulla coda corretta
void richiesta(struct gestore_t *g, int threadId, int priorità){
    printf("Sono il thread %d ed ho priorità %d\n", threadId, priorità);

    g->semPriority[priorità].count++;
    sem_wait(&g->semPriority[priorità].s);

    g->semPriority[priorità].count--;
}

//eseguo la wait su semA, uso RA e poi la rilascio
void getA(struct gestore_t *g, int threadId){
    sem_wait(&g->semA);
    printf("Sono il thread %d e sto usando RA\n", threadId);

    pausetta();
    
    printf("Sono il thread %d e sto rilasciando RA\n", threadId);
    sem_post(&g->semA);
}

//eseguo la wait su semB, uso RB e poi la rilascio
void getB(struct gestore_t *g, int threadId){
    sem_wait(&g->semB);
    printf("Sono il thread %d e sto usando RB\n", threadId);
    
    pausetta();
    
    printf("Sono il thread %d e sto rilasciando RB\n", threadId);
    sem_post(&g->semB);
}


void ricA(struct gestore_t *g, int threadId){
    richiesta(g, threadId, RA);

    sem_wait(&g->mutex);
    
    getA(g, threadId);

    sem_post(&g->mutex);
}


void ricB(struct gestore_t *g, int threadId){
    richiesta(g, threadId, RB);

    sem_wait(&g->mutex);
    
    getB(g, threadId);

    sem_post(&g->mutex);
}

//una volta preso il mutex, controllo il valore del semaforo A e B, ed eventualmente prendo o A o B
void ricQ(struct gestore_t *g, int threadId){
    richiesta(g, threadId, RQ);

    sem_wait(&g->mutex);

    int v;
    sem_getvalue(&g->semA, &v);
    if(v!=0) {
        getA(g, threadId);
    }
    else {
        getB(g, threadId);
    }

    sem_post(&g->mutex);
    
}


void ric2(struct gestore_t *g, int threadId){
    richiesta(g, threadId, R2);

    sem_wait(&g->mutex);
    
    getA(g, threadId);
    getB(g, threadId);

    sem_post(&g->mutex);
}


void gestione(struct gestore_t *g){
    int valA, valB;
    
    sem_wait(&g->mutex);

    sem_getvalue(&g->semA, &valA);
    sem_getvalue(&g->semB, &valB);

    if(g->semPriority[RQ].count > 0 && (valA!=0 && valB!=0)){
        sem_post(&g->semPriority[RQ].s);
    }
    else if(g->semPriority[RA].count > 0 && valA!=0){
        sem_post(&g->semPriority[RA].s);
    }
    else if(g->semPriority[RB].count > 0 && valB!=0){
        sem_post(&g->semPriority[RB].s);
    }
    else if(g->semPriority[R2].count > 0 && valA!=0 && valB!=0){
        sem_post(&g->semPriority[R2].s);
    }

    sem_post(&g->mutex);
    //pausetta();
}


void *execGestore(void *arg){
    for(;;){
        gestione(&gestore);
    }
}


void *execThread(void *arg){
    int scelta;
    for(;;){
        pausetta();
        scelta = rand() % 4;
        switch(scelta){
            case 0:
                ricQ(&gestore, (int) arg);
                break;

            case 1:
                ricA(&gestore, (int) arg);
                break;
        
            case 2:
                ricB(&gestore, (int) arg);
                break;
            
            case 3:
                ric2(&gestore, (int) arg);
                break;
        }
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

    pthread_create(&thread, &threadAttr, execGestore, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, execThread, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_join(thread, NULL);

    return 0;
}