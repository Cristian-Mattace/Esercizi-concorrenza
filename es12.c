#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N_RESOURCES 10
#define N 20
#define RESET -1
#define DELAY 1000


typedef enum {false, true} Boolean;

struct R_t{
    sem_t s;
    Boolean allocated;
};

struct gestore_t{
    sem_t mutex;
    sem_t clients[N];

    int client_timeout[N];

    struct R_t resources[N_RESOURCES];
    
} gestore;


void status(struct gestore_t *g){

    printf("[");
    for(int cnt=1; cnt<N_RESOURCES-1; cnt++){
        printf("%d, ", g->resources[cnt].allocated);
    }
    printf("%d ", g->resources[N_RESOURCES-1].allocated);
    printf("]\n");

}


void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);

    for(int cnt=0; cnt<N; cnt++){
        sem_init(&g->clients[cnt], 0, 0);
        g->client_timeout[cnt] = RESET;
    }


    for(int cnt=1; cnt<N_RESOURCES; cnt++){
        sem_init(&g->resources[cnt].s, 0, 1);
        g->resources[cnt].allocated = false;
    }
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%DELAY+1)*1000000;
    nanosleep(&t,NULL);
}

int checkResource(struct gestore_t *g){
    for(int cnt=1; cnt<N_RESOURCES; cnt++){
        if(!g->resources[cnt].allocated){
            g->resources[cnt].allocated = true;
            sem_wait(&g->resources[cnt].s);
            return cnt;
        }
    }

    return 0;
}


int richiesta(struct gestore_t *g, int t, int threadId){
    int r;
    sem_wait(&g->mutex);

    r = checkResource(g);

    if(r>0){
        printf("Sono il thread %d ed ho preso la risorsa %d\n", threadId, r);
        status(g);

        sem_post(&g->mutex);
    
        return r;
    }

    g->client_timeout[threadId] = t;
    printf("Sono il thread %d e non ci sono risorse disponibili\n", threadId);

    sem_post(&g->mutex);

    sem_wait(&g->clients[threadId]);

    sem_wait(&g->mutex);
    
    r = checkResource(g);

    if(r==0){
        printf("Sono il thread %d --> TIMEOUT\n", threadId);
    }
    else{
        printf("Sono il thread %d ed ho preso la risorsa %d\n", threadId, r);
        status(g);
    }

    sem_post(&g->mutex);

    return r;
}


void rilascio(struct gestore_t *g, int risorsa, int threadId){
    sem_wait(&g->mutex);

    sem_post(&g->resources[risorsa].s);
    g->resources[risorsa].allocated = false;

    for(int cnt=0; cnt<N; cnt++){
        if(g->client_timeout[cnt] != RESET){
            sem_post(&g->clients[cnt]);
            g->client_timeout[cnt] = RESET;
            break;
        }
    }

    printf("Sono il thread %d ed ho rilasciato la risorsa %d\n", threadId, risorsa);
    status(g);

    sem_post(&g->mutex);
}


void tick(struct gestore_t *g){
    sem_wait(&g->mutex);

    for(int cnt=0; cnt<N; cnt++){
        if(g->client_timeout[cnt] != RESET){
            if(g->client_timeout[cnt] == 0){
                g->client_timeout[cnt] = RESET;
                sem_post(&g->clients[cnt]);
            }
            else{
                g->client_timeout[cnt]--;
            }
        }
        //pausetta();
    }

    sem_post(&g->mutex);

    pausetta();
}


void *clients(void *arg){
    int risorsa;
    int t;
    for(;;){
        pausetta();

        t = rand() %10 + 1;
        risorsa = richiesta(&gestore, t, (int) arg);

        if(risorsa != 0){
            pausetta();
            rilascio(&gestore, risorsa, (int) arg);
        }

        pausetta();
    }
}


void *exeClock(){
    for(;;){
        tick(&gestore);
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

    pthread_create(&thread, &threadAttr, exeClock, NULL);   //DA METTERE PRIMAAAAAAAA

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, clients, (void *) i);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}