#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 5
#define N_TORTE 3

struct pasticceria_t{
    sem_t mutex;
    sem_t clienti, cuoco, commesso;

    int tortePronte;

} pasticceria;


void init_pasticceria(struct pasticceria_t *p){
    sem_init(&p->mutex, 0, 1);

    sem_init(&p->clienti, 0, 0);
    sem_init(&p->cuoco, 0, 0);
    sem_init(&p->commesso, 0, 0);

    p->tortePronte = 0;
}


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void cuoco_inizio_torta(struct pasticceria_t *p){

    sem_wait(&p->mutex);

    if(p->tortePronte < N_TORTE){
        sem_post(&p->mutex);
    }
    else{
        sem_post(&p->mutex);
        sem_wait(&p->cuoco);
    }
}


void cuoco_fine_torta(struct pasticceria_t *p){

    sem_wait(&p->mutex);

    p->tortePronte++;

    if(p->tortePronte == 1) sem_post(&p->commesso);

    printf("[TORTE : %d]\n", p->tortePronte);

    sem_post(&p->mutex);
}


/* void commesso_prendo_torta(struct pasticceria_t *p){

    sem_wait(&p->mutex);

    if(p->tortePronte == 0){
        printf("Sono il commesso, non ci sono torte!\n");
        sem_post(&p->cuoco);
        sem_post(&p->mutex);

        sem_wait(&p->commesso);

    }
    else{
        sem_post(&p->mutex);
    }
} */
void commesso_prendo_torta(struct pasticceria_t *p){

    sem_wait(&p->mutex);

    while (p->tortePronte == 0){
        printf("Sono il commesso, non ci sono torte!\n");
        sem_post(&p->cuoco);
        sem_post(&p->mutex);

        sem_wait(&p->commesso);
        sem_wait(&p->mutex);
    }
    sem_post(&p->mutex);
}


void commesso_vendo_torta(struct pasticceria_t *p){

    sem_wait(&p->mutex);

    p->tortePronte--;
    printf("Sono il commesso, torta venduta\n");
    sem_post(&p->clienti);    

    sem_post(&p->mutex);    
}


void cliente_acquisto(struct pasticceria_t *p){
    printf("Sono un cliente e sto aspettando la torta\n");
    sem_wait(&p->clienti);
}


void *cuoco(void *arg){
    for(;;){
        cuoco_inizio_torta(&pasticceria);   //1 torta, può essere bloccante
        //pausetta();
        cuoco_fine_torta(&pasticceria); //1 torta non bloccante
    }
}


void *commesso(void *arg){
    for(;;){
        commesso_prendo_torta(&pasticceria);    //bloccante
        //pausetta();
        //incarto la torta
        commesso_vendo_torta(&pasticceria);     //bloccante
    }
}


void *cliente(void *arg){
    for(;;){
        //vado in pasticceria ad acquistare una torta
        cliente_acquisto(&pasticceria);     //bloccante
        printf("Sono un cliente ed ho comprato la torta\n");
        pausetta();
        pausetta();
        //torno a casa
    }
}


int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread;
    int i;

    // init struct
    init_pasticceria(&pasticceria);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    pthread_create(&thread, &threadAttr, cuoco, NULL);

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, cliente, (void *) i);

    pthread_create(&thread, &threadAttr, commesso, NULL);
    

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+2; i++)
        pthread_join(thread, NULL);

    return 0;
}