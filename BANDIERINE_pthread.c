#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 2

struct bandierine_t{
    pthread_mutex_t mutex;
    pthread_cond_t pronti, attendiVia, attendiFine;

    int vincitore, numPronti, numTerminati;
} bandierine;


void init_bandierine(struct bandierine_t *b){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_condattr_init(&condAttr);

    pthread_mutex_init(&b->mutex, &mutexAttr);
    pthread_cond_init(&b->pronti, &condAttr);
    pthread_cond_init(&b->attendiVia, &condAttr);
    pthread_cond_init(&b->attendiFine, &condAttr);

    pthread_mutexattr_destroy(&mutexAttr);
    pthread_condattr_destroy(&condAttr);

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

    pthread_cond_broadcast(&b->pronti);

}


void attendi_il_via(struct bandierine_t *b, int n){
    printf("Sono il thread %d e sono pronto\n", n);

    pthread_mutex_lock(&b->mutex);

    b->numPronti++;

    if(b->numPronti==N) pthread_cond_signal(&b->attendiVia); //sveglio il giudice

    while(b->numPronti!=N){
        pthread_cond_wait(&b->pronti, &b->mutex);
    }
    
    pthread_mutex_unlock(&b->mutex);

    printf("Sono il thread %d e sono partito\n", n);
}


int bandierina_presa(struct bandierine_t *b, int n){
    pthread_mutex_lock(&b->mutex);

    if(b->vincitore == -1){
        b->vincitore=n;
        printf("Sono il thread %d ed ho preso la bandiera\n", n);
        pthread_mutex_unlock(&b->mutex);
        return 1;
    }
    else{
        pthread_mutex_unlock(&b->mutex);
        return 0;
    }
}


int sono_salvo(struct bandierine_t *b, int n){
    pthread_mutex_lock(&b->mutex);

    b->numTerminati++;

    if(b->numTerminati == 1){
        printf("Sono il thread %d e sono salvo\n", n);
        pthread_mutex_unlock(&b->mutex);
        return 1;
    }
    else{
        pthread_mutex_unlock(&b->mutex);
        pthread_cond_signal(&b->attendiFine);
        return 0;
    }
}


int ti_ho_preso(struct bandierine_t *b, int n){
    pthread_mutex_lock(&b->mutex);

    b->numTerminati++;

    if(b->numTerminati == 1){
        printf("Sono il thread %d ed ho rubato la bandiera\n", n);
        b->vincitore=n;
        pthread_mutex_unlock(&b->mutex);
        
        return 1;
    }
    else{
        pthread_mutex_unlock(&b->mutex);
        pthread_cond_signal(&b->attendiFine);
        return 0;
    }
}


void attendi_giocatori(struct bandierine_t *b){

    pthread_mutex_lock(&b->mutex);

    printf("Sono il giudice e sto aspettando i giocatori\n");

    while(b->numPronti != N){
        pthread_cond_wait(&b->attendiVia, &b->mutex);
    }

    printf("Sono il giudice e sto per dare il via\n");

    pthread_mutex_unlock(&b->mutex);

}


int risultato_gioco(struct bandierine_t *b){
    pthread_mutex_lock(&b->mutex);

    while(b->numTerminati != N){
        pthread_cond_wait(&b->attendiFine, &b->mutex);
    }

    pthread_mutex_unlock(&b->mutex);

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