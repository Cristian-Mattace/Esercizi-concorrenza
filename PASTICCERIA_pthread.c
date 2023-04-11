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
    pthread_mutex_t mutex;
    pthread_cond_t clienti, cuoco, commesso;

    int tortePronte;

} pasticceria;


void init_pasticceria(struct pasticceria_t *p){
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_condattr_init(&condAttr);

    pthread_mutex_init(&p->mutex, &mutexAttr);
    pthread_cond_init(&p->clienti, &condAttr);

    pthread_mutexattr_destroy(&mutexAttr);
    pthread_condattr_destroy(&condAttr);

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
    pthread_mutex_lock(&p->mutex);

    while(p->tortePronte == N_TORTE){
        pthread_cond_wait(&p->cuoco, &p->mutex);
    }

    printf("Sono il cuoco e sto facendo una torta\n");
    pthread_mutex_unlock(&p->mutex);
}


void cuoco_fine_torta(struct pasticceria_t *p){
    pthread_mutex_lock(&p->mutex);

    p->tortePronte++;

    if(p->tortePronte == 1) pthread_cond_signal(&p->commesso);

    printf("[TORTE : %d]\n", p->tortePronte);

    pthread_mutex_unlock(&p->mutex);
}


void commesso_prendo_torta(struct pasticceria_t *p){
    pthread_mutex_lock(&p->mutex);

    while(p->tortePronte == 0){
        printf("Sono il commesso, non ci sono torte!\n");
        pthread_cond_signal(&p->cuoco);
        pthread_cond_wait(&p->commesso, &p->mutex);
    }

    pthread_mutex_unlock(&p->mutex);
}


void commesso_vendo_torta(struct pasticceria_t *p){

    pthread_mutex_lock(&p->mutex);

    p->tortePronte--;
    printf("Sono il commesso, torta venduta\n");
    pthread_cond_signal(&p->clienti);

    pthread_mutex_unlock(&p->mutex);

    
}


void cliente_acquisto(struct pasticceria_t *p){
    printf("Sono un cliente e sto aspettando la torta\n");
    pthread_cond_wait(&p->clienti, &p->mutex);
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

    for(i = 0; i < N; i++)
        pthread_create(&thread, &threadAttr, cliente, (void *) i);

    pthread_create(&thread, &threadAttr, commesso, NULL);
    pthread_create(&thread, &threadAttr, cuoco, NULL);

    pthread_attr_destroy(&threadAttr);

    for(i = 0; i < N+2; i++)
        pthread_join(thread, NULL);

    return 0;
}