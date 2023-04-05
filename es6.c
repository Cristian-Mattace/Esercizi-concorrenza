#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <sched.h>

//5 filosofi e 5 forchette
#define N 5
#define DELAY 5

//stati dei filosofi
#define THINKING 0
#define EATING 1


//struttura manager contenente le forchette
struct gestore_t{
    sem_t fork[N];
} gestore;

//inizializzazione gestore con semafori forchette=1
void init_gestore(struct gestore_t *g){
    for(int i=0; i<N; i++){
        sem_init(&g->fork[i], 0, 1);
    }
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand()%DELAY+1)*1000000;
    nanosleep(&t,NULL);
}

//non fa nulla, solo una sleep
void think(struct gestore_t *g, int threadId){
    printf("THREAD %d THINKING\n", threadId);
    sleep(2);
}

//il thread 0-esimo è l'unico a prendere prima la forchetta SX poi DX
//tutti gli altri prendono prima la forchetta DX poi SX
void eat(struct gestore_t *g, int threadId){

    if(threadId == 0){
        sem_wait(&g->fork[threadId]); //SX
        sem_wait(&g->fork[threadId + 1 % N]); //DX
    }
    else{
        sem_wait(&g->fork[threadId + 1 % N]); //DX
        sem_wait(&g->fork[threadId]); //SX
    }

    printf("THREAD %d EATING (fork %d - %d)\n", threadId, threadId, threadId+1%N);

    //simulo il pasto
    pausetta();

    //libero le forchette
    sem_post(&g->fork[threadId+1%N]);
    sem_post(&g->fork[threadId]);

    printf("THREAD %d STOP EATING (release fork %d - %d)\n", threadId, threadId, threadId+1%N);

}

//funzione base del filosofo, iterativamente pensa e mangia
void *filosofo(void *arg)
{
    for(;;){
        think(&gestore, (int) arg);
        pausetta();
        eat(&gestore, (int) arg);
        pausetta();
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
        pthread_create(&thread, &attr, filosofo, (void *)i);
    }

    pthread_attr_destroy(&attr);

    for(int i=0; i<N; i++){
        pthread_join(thread, NULL);
    }

    return 0;
}