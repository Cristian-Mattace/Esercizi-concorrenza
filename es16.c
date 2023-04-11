#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define DELAY 1000
#define N 20
#define UOMO 0
#define DONNA 1
#define NESSUNO -1
#define MAX_DENTRO 5

typedef enum {false, true} Boolean;


struct gestore_t{
    sem_t mutex;
    sem_t tornello;
    sem_t attesa, semUomo, semDonna;

    int N_attesa, N_persone_dentro;
    int genere, genereAttesa;
} gestore;


void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);
    sem_init(&g->tornello, 0, 1);
    sem_init(&g->attesa, 0, 0);
    sem_init(&g->semDonna, 0, 0);
    sem_init(&g->semUomo, 0, 0);

    g->N_attesa = 0;
    g->N_persone_dentro = 0;
    g->genere = NESSUNO;
    g->genereAttesa = NESSUNO;

}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%DELAY+1)*1000000;
    nanosleep(&t,NULL);
}


void uomo(struct gestore_t *g, int threadId){
    sem_wait(&g->tornello);

    if(g->genere == NESSUNO){
        g->genere = UOMO;
    } else if(g->genere == DONNA || g->N_persone_dentro == MAX_DENTRO){
        g->genereAttesa = UOMO;
        printf("[UOMO %d] sono in attesa\n", threadId);
        sem_wait(&g->attesa);
        g->genere = UOMO;
        g->genereAttesa = NESSUNO;
    }

    sem_wait(&g->mutex);
    g->N_persone_dentro++;
    sem_post(&g->mutex);

    sem_post(&g->tornello);

    printf("[UOMO %d] sono dentro\n", threadId);
    pausetta();

    printf("[UOMO %d] sono fuori\n", threadId);

    sem_wait(&g->mutex);
    g->N_persone_dentro--;
    if(g->N_persone_dentro == MAX_DENTRO-1 && g->genereAttesa == UOMO)
        sem_post(&g->attesa);
    else if(g->N_persone_dentro == 0)
        sem_post(&g->attesa);
    sem_post(&g->mutex);
}


void donna(struct gestore_t *g, int threadId){
    sem_wait(&g->tornello);

    if(g->genere == NESSUNO){
        g->genere = DONNA;
    } else if(g->genere == UOMO || g->N_persone_dentro == MAX_DENTRO){
        g->genereAttesa = DONNA;
        printf("[DONNA %d] sono in attesa\n", threadId);
        sem_wait(&g->attesa);
        g->genere = DONNA;
        g->genereAttesa = NESSUNO;
    }

    sem_wait(&g->mutex);
    g->N_persone_dentro++;
    sem_post(&g->mutex);

    sem_post(&g->tornello);

    printf("[DONNA %d] sono dentro\n", threadId);
    pausetta();

    printf("[DONNA %d] sono fuori\n", threadId);

    sem_wait(&g->mutex);
    g->N_persone_dentro--;
    if(g->N_persone_dentro == MAX_DENTRO-1 && g->genereAttesa == DONNA)
        sem_post(&g->attesa);
    else if(g->N_persone_dentro == 0)
        sem_post(&g->attesa);
    sem_post(&g->mutex);
}


void *exeThread(void *arg){
    int genere;

    for(;;){
        genere = rand() % 2;
        if(genere == 0){
            uomo(&gestore, (int) arg);
            pausetta();
        }
        else{
            donna(&gestore, (int) arg);
            pausetta();
        }
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

    for(i = 0; i < N; i++){
        pthread_create(&thread, &threadAttr, exeThread, (void *) i);
    }

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}