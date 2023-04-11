#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 2

struct bandierine_t{
    sem_t mutex;
    sem_t pronti, attendiVia, attendiFine;

    int vincitore, numPronti, numTerminati;
} bandierine;


void init_bandierine(struct bandierine_t *b){
    sem_init(&b->mutex, 0, 1);

    sem_init(&b->attendiVia, 0, 0);
    sem_init(&b->attendiFine, 0, 0);

    sem_init(&b->pronti, 0, 0);

    b->vincitore = -1;
    b->numPronti = 0;
    b->numTerminati = 0;
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void via(struct bandierine_t *b){
    for(int cnt=0; cnt<N; cnt++){
        sem_post(&b->pronti);
    }
}


void attendi_il_via(struct bandierine_t *b, int n){
    printf("Sono il thread %d e sono pronto\n", n);
    sem_wait(&b->mutex);

    b->numPronti++;

    if(b->numPronti==N) sem_post(&b->attendiVia); //sveglio il giudice

    sem_post(&b->mutex);

    sem_wait(&b->pronti);

    printf("Sono il thread %d e sono partito\n", n);
}


int bandierina_presa(struct bandierine_t *b, int n){
    sem_wait(&b->mutex);

    if(b->vincitore == -1){
        b->vincitore=n;
        printf("Sono il thread %d ed ho preso la bandiera\n", n);
        sem_post(&b->mutex);
        return 1;
    }
    else{
        sem_post(&b->mutex);
        return 0;
    }
}


int sono_salvo(struct bandierine_t *b, int n){
    sem_wait(&b->mutex);

    b->numTerminati++;

    if(b->numTerminati == 1){
        printf("Sono il thread %d e sono salvo\n", n);
        sem_post(&b->mutex);
        return 1;
    }
    else{
        sem_post(&b->mutex);
        sem_post(&b->attendiFine);
        return 0;
    }
}


int ti_ho_preso(struct bandierine_t *b, int n){
    sem_wait(&b->mutex);

    b->numTerminati++;

    if(b->numTerminati == 1){
        printf("Sono il thread %d ed ho rubato la bandiera\n", n);
        b->vincitore=n;
        sem_post(&b->mutex);
        
        return 1;
    }
    else{
        sem_post(&b->mutex);
        sem_post(&b->attendiFine);
        return 0;
    }
}


void attendi_giocatori(struct bandierine_t *b){
    printf("Sono il giudice e sto aspettando i giocatori\n");
    sem_wait(&b->attendiVia);

    printf("Sono il giudice e sto per dare il via\n");
}


int risultato_gioco(struct bandierine_t *b){
    sem_wait(&b->attendiFine);

    return b->vincitore;
}


void *giocatore(void *arg){
    int numerogiocatore = (int) arg;

    attendi_il_via(&bandierine, numerogiocatore); //bloccante
    //corro
    if(bandierina_presa(&bandierine, numerogiocatore)){
        //corro alla base
        pausetta();
        if(sono_salvo(&bandierine, numerogiocatore)) printf("");
    }
    else{
        //cerco di acchiapapre l'altro
        pausetta();
        if(ti_ho_preso(&bandierine, numerogiocatore)) printf("");
    }

    return 0;
}


void *giudice(void *arg){
    attendi_giocatori(&bandierine); //bloccante
    //do il via
    via(&bandierine);   //sblocca i giocatori
    printf("Il vincitore è: %d\n", risultato_gioco(&bandierine));

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

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, giocatore, (void *) i);

    pthread_create(&thread, &threadAttr, giudice, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}