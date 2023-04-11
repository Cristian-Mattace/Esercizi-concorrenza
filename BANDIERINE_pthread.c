#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 2
#define NESSUNO -1

typedef enum {false, true} Boolean;

struct bandierine_t{
    pthread_mutex_t mutex;
    pthread_cond_t giocatoriPronti, attendiGiocatoriVia, attendiGiocatoriFine;


    int vincitore, nGiocatoriPronti, nGiocatoriTerminati;
    
} bandierine;


void init_bandierine(struct bandierine_t *b){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_condattr_init(&condAttr);

    pthread_mutex_init(&b->mutex, &mutexAttr);
    pthread_cond_init(&b->giocatoriPronti, &condAttr);
    pthread_cond_init(&b->attendiGiocatoriVia, &condAttr);
    pthread_cond_init(&b->attendiGiocatoriFine, &condAttr);

    pthread_mutexattr_destroy(&mutexAttr);
    pthread_condattr_destroy(&condAttr);

    b->vincitore = NESSUNO;
    b->nGiocatoriPronti = 0;
    b->nGiocatoriTerminati = 0;
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void via(struct bandierine_t *b){
    
    pthread_cond_broadcast(&b->giocatoriPronti);

}


void attendi_il_via(struct bandierine_t *b, int n){
    printf("Sono il giocatore %d e sto aspettando il via\n", n);

    pthread_mutex_lock(&b->mutex);

    b->nGiocatoriPronti++;

    if(b->nGiocatoriPronti == 2) pthread_cond_signal(&b->attendiGiocatoriVia);

    while(b->nGiocatoriPronti != 2){
        pthread_cond_wait(&b->giocatoriPronti, &b->mutex);
    }

    pthread_mutex_unlock(&b->mutex);
}


int bandierina_presa(struct bandierine_t *b, int n){
    
    pthread_mutex_lock(&b->mutex);

    if(b->vincitore == NESSUNO){
        b->vincitore = n;
        pthread_mutex_unlock(&b->mutex);
        return 1;
    }
    else {
        pthread_mutex_unlock(&b->mutex);
        return 0;
    }

}


int sono_salvo(struct bandierine_t *b, int n){
    
    pthread_mutex_lock(&b->mutex);

    b->nGiocatoriTerminati++;

    if(b->nGiocatoriTerminati == 1){
        pthread_mutex_unlock(&b->mutex);
        return 1;
    }
    else {
        pthread_mutex_unlock(&b->mutex);
        pthread_cond_signal(&b->attendiGiocatoriFine);
        return 0;
    }
}


int ti_ho_preso(struct bandierine_t *b, int n){

    pthread_mutex_lock(&b->mutex);

    b->nGiocatoriTerminati++;

    if(b->nGiocatoriTerminati == 1){
        b->vincitore = n;
        pthread_mutex_unlock(&b->mutex);
        return 1;
    }
    else {
        pthread_mutex_unlock(&b->mutex);
        pthread_cond_signal(&b->attendiGiocatoriFine);
        return 0;
    }
}


int risultato_gioco(struct bandierine_t *b){
    pthread_mutex_lock(&b->mutex);

    printf("Sono il giudice e sto aspettando i giocatori per terminare\n");

    while(b->nGiocatoriTerminati != 2){
        pthread_cond_wait(&b->attendiGiocatoriFine, &b->mutex);
    }

    int winner = b->vincitore;

    pthread_mutex_unlock(&b->mutex);

    return winner;
}


void attendi_giocatori(struct bandierine_t *b){
    pthread_mutex_lock(&b->mutex);

    printf("Sono il giudice e sto aspettando i giocatori per dare il via\n");

    while(b->nGiocatoriPronti != 2){
        pthread_cond_wait(&b->attendiGiocatoriVia, &b->mutex);
    }

    pthread_mutex_unlock(&b->mutex);
}


void *giudice(void *arg){
    attendi_giocatori(&bandierine);
    //pronti, partenza
    via(&bandierine);

    printf("Il vincitore è: %d!!!\n", risultato_gioco(&bandierine));
    
    return 0;
}


void *giocatore(void *arg) {
    int numerogiocatore = (int) arg;

    attendi_il_via(&bandierine, numerogiocatore);
    //CORRO
    if(bandierina_presa(&bandierine, numerogiocatore)){
        pausetta();
        if(sono_salvo(&bandierine, numerogiocatore)) printf("SONO SALVO!\n");
    }
    else
    {
        //cerco di acchiappare l'altro
        pausetta();
        if(ti_ho_preso(&bandierine, numerogiocatore)) printf("TI HO PRESO\n");
    }

    return 0;
}


int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    init_bandierine(&bandierine);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    
    for(i = 0; i <N; i++)
        pthread_create(&thread, &threadAttr, giocatore, (void *) i);

    pthread_create(&thread, &threadAttr, giudice, NULL);
    

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}
