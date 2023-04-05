#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <sched.h>

#define BARBERS 3
#define DELAY 100
#define N 10

struct ingresso_t{
    sem_t divano;
    sem_t uscio;
    
} ingresso;

//struttura manager contenente i barbieri, il cassiere, il divano e l'uscio
struct gestore_t{
    sem_t mutex;

    sem_t barbiere[BARBERS];
    sem_t cassiere;

    struct ingresso_t ingresso; 

} gestore;

//inizializzazione gestore con semafori
//inizializzo il divano a 3 come il numero di barbieri, quindi ne passeranno 3, mentre gli altri si bloccano
//inizializzo l'uscio a 4, così ne passeranno 4 che si bloccheranno sul divano, gli altri sull'uscio
void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);

    sem_init(&g->ingresso.divano, 0, 3);
    sem_init(&g->ingresso.uscio, 0, 4);

    sem_init(&g->cassiere, 0, 1);

    for(int i=0; i<BARBERS; i++){
        sem_init(&g->barbiere[i], 0, 1);
    }
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand()%DELAY+1)*1000000;
    nanosleep(&t,NULL);
}


//pago al cassiere
void pagamento(struct gestore_t *g, int threadId){
    printf("Sono il thread %d e sto per pagare\n", threadId);
    
    sem_wait(&g->cassiere);

    printf("Sono il thread %d e sto pagando\n", threadId);
    pausetta();
    printf("Sono il thread %d ed ho pagato\n", threadId);

    sem_post(&g->cassiere);
}

//taglio i capelli
void taglio(struct gestore_t *g, int threadId){
    //mi sto alzando, quindi libero un posto
    sem_post(&g->ingresso.uscio);

    int cnt = 0;

    //prendo il mutex
    sem_wait(&g->mutex);
    for(cnt=0; cnt<BARBERS; cnt++){
        int semValue;
        sem_getvalue(&g->barbiere[cnt], &semValue);

        if(semValue!=0){
            printf("Sono il thread %d e mi sto tagliando i capelli sul barbiere %d\n", threadId, cnt);
            sem_wait(&g->barbiere[cnt]);
            break;
        }
    }
    //rilascio il mutex
    sem_post(&g->mutex);

    //taglio i capelli
    pausetta();

    //rilascio il barbiere, lo rimetto a 1
    sem_post(&g->barbiere[cnt]);
    //una volta liberato il barbiere, segnalo al divano la presenza di un barbiere libero
    sem_post(&g->ingresso.divano);
}


//entra primo sull'uscio poi sul divano
void entrata(struct gestore_t *g, int threadId){
    printf("Sono %d e sto aspettando fuori\n", threadId);
    sem_wait(&g->ingresso.uscio);
    printf("Sono %d e sto aspettando sul divano\n", threadId);
    sem_wait(&g->ingresso.divano);
}


//funzione base del cliente
void *customer(void *arg)
{
    entrata(&gestore, (int) arg);

    taglio(&gestore, (int) arg);

    pagamento(&gestore, (int) arg);

    pausetta();

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
        pthread_create(&thread, &attr, customer, (void *)i);
    }

    pthread_attr_destroy(&attr);

    for(int i=0; i<N; i++){
        pthread_join(thread, NULL);
    }

    return 0;
}