#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5

typedef enum {false, true} Boolean;

struct corsa_t{
    pthread_mutex_t mutex;
    pthread_cond_t runners, condStart, condEnd;

    int first, last;

    int num_ready, num_arrived;
    
} corsa;


void init_corsa(struct corsa_t *c){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_condattr_init(&condAttr);

    pthread_mutex_init(&c->mutex, &mutexAttr);
    pthread_cond_init(&c->runners, &condAttr);
    pthread_cond_init(&c->condStart, &condAttr);
    pthread_cond_init(&c->condEnd, &condAttr);

    pthread_mutexattr_destroy(&mutexAttr);
    pthread_condattr_destroy(&condAttr);

    c->first = -1;
    c->last = -1;
    c->num_ready = 0;
    c->num_arrived = 0;
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void corridore_attendivia(struct corsa_t *c, int numerocorridore){
    pthread_mutex_lock(&c->mutex);

    c->num_ready++;

    if(c->num_ready == N) pthread_cond_signal(&c->condStart);


    while(c->num_ready != N){
        pthread_cond_wait(&c->runners, &c->mutex);
    }

    pthread_mutex_unlock(&c->mutex);

}


void corridore_arrivo(struct corsa_t *c, int numerocorridore){
    pthread_mutex_lock(&c->mutex);

    c->num_arrived++;

    if(c->first == -1) c->first = numerocorridore;
    else if(c->last == -1 && c->num_arrived == N){
        c->last = numerocorridore;
        pthread_cond_signal(&c->condEnd);
    }

    pthread_mutex_unlock(&c->mutex);

}


void arbitro_attendicorridori(struct corsa_t *c){
    pthread_mutex_lock(&c->mutex);

    while(c->num_ready != N){
        pthread_cond_wait(&c->condStart, &c->mutex);
    }

    pthread_mutex_unlock(&c->mutex);
}


void arbitro_via(struct corsa_t *c){

    pthread_cond_broadcast(&c->runners);
}


void arbitro_risultato(struct corsa_t *c){
    pthread_mutex_lock(&c->mutex);

    while(c->num_arrived != N){
        pthread_cond_wait(&c->condEnd, &c->mutex);
    }

    pthread_mutex_unlock(&c->mutex);

    printf("FINE GARA!!! [PRIMO: %d] [ULTIMO: %d]\n", c->first, c->last);
}


void *referee(void *arg){
    printf("Sono l'arbitro e sto andando in pista\n");
    arbitro_attendicorridori(&corsa); // bloccante
    printf("Sono l'arbitro e sto per dare il via\n");
    arbitro_via(&corsa); // non bloccante
    printf("Sono l'arbitro e sto aspettando che tutti arrivino\n");
    arbitro_risultato(&corsa); // bloccante

    return 0;
}


void *runners(void *arg) {
    printf("Sono il corridore %d e sto andando sulla pista\n", (int) arg);
    corridore_attendivia(&corsa, (int) arg); // bloccante
    printf("Sono il corridore %d e sto correndo\n", (int) arg);
    pausetta();
    corridore_arrivo(&corsa, (int) arg); // non bloccante
    printf("Sono il corridore %d e sto andando a casa\n", (int) arg);

    return 0;
}


int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    init_corsa(&corsa);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, runners, (void *) i);

    pthread_create(&thread, &threadAttr, referee, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+1; i++)
        pthread_join(thread, NULL);

    return 0;
}