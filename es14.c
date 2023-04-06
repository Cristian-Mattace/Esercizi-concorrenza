#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 10
#define NON_ESEGUITO 0
#define BLOCCATO 1
#define ESEGUITO 2
#define DELAY 1000


typedef enum {false, true} Boolean;

struct processo_t{
    sem_t s;
    int statoA, statoB;
};

struct gestore_t{
    sem_t mutex;
    sem_t semA, semB, semCD;
    sem_t semEND;

    int attiviC, attiviD, terminati, bloccatiC, bloccatiD;

    struct processo_t processo[N];
    
} gestore;


void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);

    sem_init(&g->semA, 0, 1);
    sem_init(&g->semB, 0, 1);
    sem_init(&g->semCD, 0, 1);

    sem_init(&g->semEND, 0, 0);

    g->attiviC = 0;
    g->attiviD = 0;
    g->terminati = 0;
    g->bloccatiC = 0;
    g->bloccatiD = 0;

    for(int cnt=0; cnt<N; cnt++){
        sem_init(&g->processo[cnt].s, 0, 0);
        g->processo[cnt].statoA = NON_ESEGUITO;
        g->processo[cnt].statoB = NON_ESEGUITO;
    }
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%DELAY+1)*1000000;
    nanosleep(&t,NULL);
}


void A(struct gestore_t *g, int threadId){
    int v;

    sem_wait(&g->mutex);

    sem_getvalue(&g->semA, &v);

    if(v){
        sem_post(&g->mutex);
    }
    else{
        sem_post(&g->mutex);

        g->processo[threadId].statoA = BLOCCATO;
        printf("Sono il thread %d e sono bloccato su A\n", threadId);
        sem_wait(&g->processo[threadId].s);
    }

    sem_wait(&g->semA);
    g->processo[threadId].statoA = ESEGUITO;
    printf("Sono il thread %d e sto eseguendo A\n", threadId);
    pausetta();
    printf("Sono il thread %d e sto terminando A\n", threadId);

    for(int cnt=0; cnt<N; cnt++){
        if(g->processo[cnt].statoA == BLOCCATO){
            printf("Sono il thread %d ed ho svegliato %d in A\n", threadId, cnt);
            sem_post(&g->processo[cnt].s);
            break;
        }
    }

    sem_post(&g->semA);
}


void B(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);

    if(g->processo[threadId-1].statoB != ESEGUITO && threadId-1>0){
        sem_post(&g->mutex);

        g->processo[threadId].statoB = BLOCCATO;
        printf("Sono il thread %d e sono bloccato su B\n", threadId);
        sem_wait(&g->processo[threadId].s);
    }
    else{
        sem_post(&g->mutex);
    }

    sem_wait(&g->semB);
    g->processo[threadId].statoB = ESEGUITO;
    printf("Sono il thread %d e sto eseguendo B\n", threadId);
    pausetta(); 
    printf("Sono il thread %d e sto terminando B\n", threadId);

    if(g->processo[threadId+1].statoB == BLOCCATO && threadId+1<N){
        
        printf("Sono il thread %d ed ho svegliato %d in B\n", threadId, threadId+1);
        sem_post(&g->processo[threadId+1].s);
    }
    
    sem_post(&g->semB);
    
}


void C(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);
    
    if(g->attiviC == 0 && g->attiviD == 0){
        g->attiviC++;
        sem_wait(&g->semCD);
    }
    else if(g->attiviC > 0){
        g->attiviC++;
    }
    else if(g->attiviD > 0){
        printf("Sono il thread %d voglio C, ma è in esecuzione D, mi sospendo\n", threadId);
        g->bloccatiC++;
        sem_post(&g->mutex);
        sem_wait(&g->semCD);

        sem_wait(&g->mutex);
        g->attiviC++;
        g->bloccatiC--;
    }

    sem_post(&g->mutex);

    printf("Sono il thread %d e sto eseguendo C\n", threadId);
    pausetta();

    printf("Sono il thread %d e sto terminando C\n", threadId);
    sem_wait(&g->mutex);
    g->attiviC--;
    if(g->attiviC==0 && g->attiviD>0){
        for(int cnt=0; cnt<g->bloccatiD; cnt++){
            sem_post(&g->semCD);
        }
    }
    else if(g->attiviC==0){
        sem_post(&g->semCD);
    }

    sem_post(&g->mutex);
}


void D(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);
    
    if(g->attiviC == 0 && g->attiviD == 0){
        g->attiviD++;
        sem_wait(&g->semCD);
    }
    else if(g->attiviD > 0){
        g->attiviD++;
    }
    else if(g->attiviC > 0){
        printf("Sono il thread %d voglio D, ma è in esecuzione C, mi sospendo\n", threadId);
        g->bloccatiD++;
        sem_post(&g->mutex);
        sem_wait(&g->semCD);

        sem_wait(&g->mutex);
        g->attiviD++;
        g->bloccatiD--;
    }

    sem_post(&g->mutex);

    printf("Sono il thread %d e sto eseguendo D\n", threadId);
    pausetta();

    printf("Sono il thread %d e sto terminando D\n", threadId);
    sem_wait(&g->mutex);
    g->attiviD--;
    if(g->attiviD==0 && g->attiviC>0){
        for(int cnt=0; cnt<g->bloccatiC; cnt++){
            sem_post(&g->semCD);
        }
    }
    else if(g->attiviD==0){
        sem_post(&g->semCD);
    }

    sem_post(&g->mutex);
}


void fine_ciclo(struct gestore_t *g, int threadId){
    sem_wait(&g->mutex);

    g->terminati++;
    
    if(g->terminati == N){
        for(int cnt=0; cnt<N; cnt++){
            sem_post(&g->semEND);
        }
        g->terminati = 0;
    }
    sem_post(&g->mutex);

    printf("Sono il thread %d, ho finito e sono bloccato\n", threadId);
    sem_wait(&g->semEND);
    printf("Sono il thread %d, ho terminato\n", threadId);
}


void *exeThread(void *arg){
    int random;
    for(;;){
        A(&gestore, (int) arg);
        B(&gestore, (int) arg);

        random = rand() % 2;
        if(random % 2 == 0) C(&gestore, (int) arg);
        else D (&gestore, (int) arg); 

        fine_ciclo(&gestore, (int) arg);
        pausetta();
    }
}


int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    init_gestore(&gestore);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, exeThread, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}