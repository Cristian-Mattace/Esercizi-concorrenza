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
    sem_t mutex;
    sem_t runners;

    sem_t sem_start, sem_end;
    int num_ready, num_arrived, first, last;
} corsa;


void init_corsa(struct corsa_t *c){
    sem_init(&c->mutex, 0, 1);

    sem_init(&c->runners, 0, 0);

    sem_init(&c->sem_start, 0, 0);
    sem_init(&c->sem_end, 0, 0);

    c->num_ready = 0;
    c->num_arrived = 0;
    c->first = -1;
    c->last = -1;
    
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void corridore_attendivia(struct corsa_t *c, int numerocorridore){
    sem_wait(&c->mutex);

    c->num_ready++;

    printf("Sono il corridore %d e sto aspettando il via\n", numerocorridore);

    if(c->num_ready == N){
        sem_post(&c->sem_start); //sblocco l'arbitro
    }

    sem_post(&c->mutex);

    sem_wait(&c->runners); //i corridori di bloccano qua, TUTTI
}


void corridore_arrivo(struct corsa_t *c, int numerocorridore){
    sem_wait(&c->mutex);

    c->num_arrived++;

    if(c->first == -1) c->first = numerocorridore;
    else if(c->num_arrived == N){
        c->last = numerocorridore;
        sem_post(&c->sem_end); //sveglio l'arbitro
    } 

    sem_post(&c->mutex);
}


void arbitro_attendicorridori(struct corsa_t *c){
    sem_wait(&c->sem_start); //bloccante, attende la post dall'ultimo

}


void arbitro_via(struct corsa_t *c){
    for(int cnt=0; cnt<N; cnt++){
        sem_post(&c->runners);
    }
}


void arbitro_risultato(struct corsa_t *c){
    sem_wait(&c->sem_end);

    sem_wait(&c->mutex);

    printf("FINE GARA!!! [PRIMO: %d] [ULTIMO: %d]\n", c->first, c->last);

    sem_post(&c->mutex);
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

//funzione eseguita dai thread di default
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