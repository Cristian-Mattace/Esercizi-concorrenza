#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <sched.h>

#define N 10


typedef struct{
    char *mex;
    char *threadId;
} Envelope;


struct gestore_t{
    pthread_mutex_t mutex;
    pthread_cond_t condSEND, condRECEIVE;

    Envelope envelopes[N];
    int envIndex; //next empty cell

} gestore;


void printStatus(struct gestore_t *g){
    printf("\n[");
    for(int i=0; i<N; i++){
        printf("( %d: msg=%s; threadID=%s)\n", i, g->envelopes[i].mex, g->envelopes[i].threadId);
    }
    printf("]\n\n");
}

//ritorna il riferimento alla prima busta vuota
Envelope *getEmptyEnv(struct gestore_t *g){
    if(g->envIndex < N)
        return &g->envelopes[g->envIndex];

    return NULL;
}

//ritorna il riferimento alla prima busta piena
Envelope getEnv(struct gestore_t *g){
    Envelope eAPP = g->envelopes[0];


    for(int i=1; i<g->envIndex; i++){
        g->envelopes[i-1] = g->envelopes[i];
    }

    //aggiorno il valore di envIndex e svuoto il contenuto dell'ultimo
    g->envelopes[--g->envIndex].mex = "(null)";
    g->envelopes[g->envIndex].threadId = "(null)";

    return eAPP;
}



void init_gestore(struct gestore_t *g){
    pthread_mutexattr_t m_attr;
    pthread_condattr_t c_attr;

    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);

    pthread_mutex_init(&g->mutex, &m_attr);
    pthread_cond_init(&g->condSEND, &c_attr);
    pthread_cond_init(&g->condRECEIVE, &c_attr);

    pthread_condattr_destroy(&c_attr);
    pthread_mutexattr_destroy(&m_attr);

    g->envIndex = 0;
}


void send(struct gestore_t *g, char *threadId){

    pthread_mutex_lock(&g->mutex);

    while(g->envIndex >= N)
        pthread_cond_wait(&g->condSEND, &g->mutex);

    Envelope *e = getEmptyEnv(g);
    /* int num = rand() % 1000;
    char numChar[4];
    sprintf(numChar, "%d", num);  */

    e->mex = "MEX";
    e->threadId = threadId;

    g->envIndex++;

    printf("Thread %s send msg: %s\n", threadId, e->mex);

    printStatus(g);

    pthread_cond_signal(&g->condRECEIVE);

    pthread_mutex_unlock(&g->mutex);
}
 

void receive(struct gestore_t *g, char *threadId){

    pthread_mutex_lock(&g->mutex);

    while(g->envIndex <= 0)
        pthread_cond_wait(&g->condRECEIVE, &g->mutex);

    Envelope e = getEnv(g);

    printf("Thread %s receive msg: %s\n", threadId, e.mex);

    printStatus(g);

    pthread_cond_signal(&g->condSEND);

    pthread_mutex_unlock(&g->mutex);    
}




void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}



/* i thread */
void *receiver(void *arg)
{
    for(;;){
        receive(&gestore, arg);
        sleep(1);
        pausetta();
    }
    return 0;
}

void *sender(void *arg)
{
    for(;;){
        send(&gestore, arg);
        sleep(1);
        pausetta();
    }
    return 0;
}



int main (int argc, char **argv) {
    pthread_attr_t lowAttr, mediumAttr, highAttr;
    struct sched_param lowPolicy, mediumPolicy, highPolicy;
    pthread_t thread;
    

    /* inizializzo il mio sistema */
    init_gestore(&gestore);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    //non c'è bisogno di aspettare la fine del thread con la join
    pthread_attr_setdetachstate(&lowAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&mediumAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&highAttr, PTHREAD_CREATE_DETACHED);

    pthread_attr_init(&lowAttr);
    pthread_attr_setschedpolicy(&lowAttr, SCHED_FIFO);
    lowPolicy.sched_priority = 2;
    pthread_attr_setschedparam(&lowAttr, &lowPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    //pthread_attr_setinheritsched(&lowAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_init(&mediumAttr);
    pthread_attr_setschedpolicy(&mediumAttr, SCHED_FIFO);
    mediumPolicy.sched_priority = 1;
    pthread_attr_setschedparam(&mediumAttr, &mediumPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    //pthread_attr_setinheritsched(&mediumAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_init(&highAttr);
    pthread_attr_setschedpolicy(&highAttr, SCHED_FIFO);
    highPolicy.sched_priority = 0;
    pthread_attr_setschedparam(&highAttr, &highPolicy);  
    // NTPL defaults to INHERIT_SCHED!!!
    //pthread_attr_setinheritsched(&highAttr, PTHREAD_EXPLICIT_SCHED);

    pthread_create(&thread, &lowAttr, receiver, (void *)"R2");
    pthread_create(&thread, &mediumAttr, receiver, (void *)"R1");
    pthread_create(&thread, &highAttr, receiver, (void *)"R0");

    pthread_create(&thread, &lowAttr, sender, (void *)"S2");
    pthread_create(&thread, &mediumAttr, sender, (void *)"S1");
    pthread_create(&thread, &highAttr, sender, (void *)"S0");

    pthread_attr_destroy(&lowAttr);
    pthread_attr_destroy(&mediumAttr);
    pthread_attr_destroy(&highAttr);


    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);

    return 0;
}