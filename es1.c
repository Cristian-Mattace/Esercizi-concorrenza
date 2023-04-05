#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define sequenza_NESSUNO 0
#define sequenza_A 1
#define sequenza_B 2
#define sequenza_C 3
#define sequenza_D 4
#define sequenza_E 5
#define sequenza_D_o_E 6

//#define USA_SEM 1 //serve per usare la IFDEF USA_SEM
#define USA_MUTEX 1 //serve per usare la IFDEF USA_MUTEX

#ifdef USA_SEM

struct gestore_t{
    sem_t mutex;

    sem_t sa, sb, sc, sd, se; //un semaforo privato per ogni procedura
    int ca, cb, cc, cd, ce; //conta quanti thread sono bloccati su quella procedura

    //stato sistema
    int nb; //numero di procedure B in esecuzione
    int stato;  //traccio lo stato del sistema
} gestore;


void init_gestore(struct gestore_t *g){
    sem_init(&g->mutex, 0, 1);

    sem_init(&g->sa, 0, 0);
    sem_init(&g->sb, 0, 0);
    sem_init(&g->sc, 0, 0);
    sem_init(&g->sd, 0, 0);
    sem_init(&g->se, 0, 0);

    g->ca = g->cb = g->cc = g->cd = g->ce = 0;

    //stato sistema
    g->nb=0;    //numero di b attivi
    g->stato = sequenza_NESSUNO;
}

void Sveglia_A_o_C(struct gestore_t *g){
    //sicuramente chiamata ad ogni fine procedura, quindi dopo B, D, E

    //decremento il numero, cambio lo stato ed eseguo la post

    if(g->ca){
        g->ca--;
        g->stato = sequenza_A;
        sem_post(&g->sa);
    }
    else if(g->cc){
        g->cc--;
        g->stato = sequenza_C;
        sem_post(&g->sc);
    }
    else{
        g->stato = sequenza_NESSUNO; //se non c'è nessuno mi metto in attesa di eventuali A o C
    }
}


void StartA(struct gestore_t *g)
{
    sem_wait(&g->mutex);
    if (g->stato == sequenza_NESSUNO) {
        /* non mi blocco! */
        g->stato = sequenza_A;
        sem_post(&g->sa);   //post preventiva in modo da non bloccarmi alla wait successiva
    }
    else {
        /* mi blocco */
        g->ca++;
    }
    
    sem_post(&g->mutex);
    sem_wait(&g->sa);   //se non mi sono bloccato, allora questa wait sarà passante, altrimenti bloccante
}

void EndA(struct gestore_t *g)  //non è bloccante
{
    sem_wait(&g->mutex);

    g->stato = sequenza_B;  //sicuramente questo va in sequenza B, e lo faccio qui per rispettare i vincoli

    /* devo svegliare TUTTI i B pendenti */
    while (g->cb) {
        g->cb--;
        g->nb++;
        sem_post(&g->sb);
    }

    sem_post(&g->mutex);
}


void StartB(struct gestore_t *g) //se mi blocco mi basta incrementare cb, perchè sarà endA a farmi partire
{
    sem_wait(&g->mutex);
    if (g->stato == sequenza_B) {
        //se lo stato è sequenza_B, inseriso un altro B
        g->nb++;
        sem_post(&g->sb);   //post preventiva, così con la wait successiva non si bloccherà
    }
    else {
        g->cb++; //incremento il numero di B bloccati
    }
    
    sem_post(&g->mutex);
    sem_wait(&g->sb);
}

void EndB(struct gestore_t *g){
    //mi basta semplicemente decrementare nb, poi controllo che se è =0 chiamo la startAoC

    sem_wait(&g->mutex);

    g->nb--;    //decremento il numero di B

    if (g->nb == 0) //se sono l'ultimo, farò qualcosa di diverso
        Sveglia_A_o_C(g);

    sem_post(&g->mutex);
}


void StartC(struct gestore_t *g)    //uguale a startA
{
    sem_wait(&g->mutex);
    if (g->stato == sequenza_NESSUNO) {
        g->stato = sequenza_C;
        sem_post(&g->sc);
    }
    else {
        g->cc++;
    } 
    sem_post(&g->mutex);
    sem_wait(&g->sc);
}

void EndC(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    if (g->cd) {
        g->cd--;
        g->stato = sequenza_D;
        sem_post(&g->sd);
    } 
    else if (g->ce) {
        g->ce--;
        g->stato = sequenza_E;
        sem_post(&g->se);
    }
    else
        /* se non c'e' nessuno bloccato, so comunque che potra' entrare
        solo un D o un E */
        g->stato = sequenza_D_o_E;

    sem_post(&g->mutex);
}


void StartD(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    //mi preoccupo solo della sequenza D_o_E perchè per la sequenza D è già stata fatta la post sul semaforo nell'endC
    if (g->stato == sequenza_D_o_E) {
        g->stato = sequenza_D;
        sem_post(&g->sd);
    }
    else {
        g->cd++;
    } 
    sem_post(&g->mutex);
    sem_wait(&g->sd);
}

void EndD(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    Sveglia_A_o_C(g);

    sem_post(&g->mutex);
}


void StartE(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    //mi preoccupo solo della sequenza D_o_E perchè per la sequenza E è già stata fatta la post sul semaforo nell'endC
    if (g->stato == sequenza_D_o_E) {
        g->stato = sequenza_E;
        sem_post(&g->se);
    }
    else {
        g->ce++;
    } 
    sem_post(&g->mutex);
    sem_wait(&g->se);
}

void EndE(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    Sveglia_A_o_C(g);

    sem_post(&g->mutex);
}

#endif


#ifdef USA_MUTEX

/* questa implementazione ricalca le strutture dati definite nella
   versione con i semafori. anche qui le variabili condition per (A e
   C) e per (D ed E) potrebbero essere unificate. */

/* la struttura condivisa */
struct gestore_t {
    pthread_mutex_t mutex;

    pthread_cond_t sa,sb,sc,sd,se;
    int ca,cb,cc,cd,ce;

    /* stato del sistema */
    int nb;  
    int stato;
} gestore;

/* inizializzazione della struttura condivisa */
void init_gestore(struct gestore_t *g)
{
    pthread_mutexattr_t m_attr;
    pthread_condattr_t c_attr;

    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);

    pthread_mutex_init(&g->mutex, &m_attr);
    pthread_cond_init(&g->sa, &c_attr);
    pthread_cond_init(&g->sb, &c_attr);
    pthread_cond_init(&g->sc, &c_attr);
    pthread_cond_init(&g->sd, &c_attr);
    pthread_cond_init(&g->se, &c_attr);

    pthread_condattr_destroy(&c_attr);
    pthread_mutexattr_destroy(&m_attr);

    g->ca = g->cb = g->cc = g->cd = g->ce = 0;

    /* stato del sistema */
    g->nb = 0;
    g->stato = sequenza_NESSUNO;
}

void StartA(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    while (g->stato != sequenza_NESSUNO) {
        /* mi blocco */
        g->ca++;
        pthread_cond_wait(&g->sa,&g->mutex);    //quando verrà sbloccato, ripartirà da qui
        g->ca--;
    }

    /* non mi blocco! */
    g->stato = sequenza_A;

    pthread_mutex_unlock(&g->mutex);
}

void EndA(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    g->stato = sequenza_B;

    /* devo svegliare tutti i B pendenti */
    if (g->cb)
        pthread_cond_broadcast(&g->sb);

    pthread_mutex_unlock(&g->mutex);
}

void StartB(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    while (g->stato != sequenza_B) {
        g->cb++;
        pthread_cond_wait(&g->sb,&g->mutex);
        g->cb--;
    }

    /* devo contare i B perchè ce ne possono essere + di 1 */
    g->nb++;

    pthread_mutex_unlock(&g->mutex);
}

void sveglia_A_o_C(struct gestore_t *g)
{
    /* chiamata alla fine di B, D o E per controllare se devo svegliare
        qualcuno alla fine di una sequenza */
    if (g->ca) {
        pthread_cond_signal(&g->sa);
    } 
    else if (g->cc) {
        pthread_cond_signal(&g->sc);
    }

    // Nota: non è nella parte else!!! 
    // dato che la startA lavora solo se lo stato è nessuno, devo settarlo obbligatoriamente, altrimenti non va avantis
    g->stato = sequenza_NESSUNO;
}

void EndB(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    g->nb--;
    if (g->nb == 0)
        sveglia_A_o_C(g);

    pthread_mutex_unlock(&g->mutex);
}

void StartC(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);
    while (g->stato != sequenza_NESSUNO) {
        g->cc++;
        pthread_cond_wait(&g->sc,&g->mutex);
        g->cc--;
    }
    g->stato = sequenza_C;
    pthread_mutex_unlock(&g->mutex);
}

void EndC(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    if (g->cd) {
        pthread_cond_signal(&g->sd);
    } 
    else if (g->ce) {
        pthread_cond_signal(&g->se);
    }

    /* a questo punto ho svegliato qualcuno dei thread bloccati, quindi
        mi metto in uno stato di attesa in cui può entrare solo un D o un
        E */
    g->stato = sequenza_D_o_E;

    pthread_mutex_unlock(&g->mutex);
}

void StartD(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);
    while (g->stato != sequenza_D_o_E) {
        g->cd++;
        pthread_cond_wait(&g->sd,&g->mutex);
        g->cd--;
    }

    g->stato = sequenza_D;

    pthread_mutex_unlock(&g->mutex);
}

void EndD(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    sveglia_A_o_C(g);

    pthread_mutex_unlock(&g->mutex);
}

void StartE(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);
    while (g->stato != sequenza_D_o_E) {
        g->ce++;
        pthread_cond_wait(&g->se,&g->mutex);
        g->ce--;
    }
    g->stato = sequenza_E;
    pthread_mutex_unlock(&g->mutex);
}

void EndE(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    sveglia_A_o_C(g);

    pthread_mutex_unlock(&g->mutex);
}

#endif


void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand()%10+1)*1000000;
  nanosleep(&t,NULL);
}


/* i thread */
void *A(void *arg)
{
  for (;;) {
    StartA(&gestore);
    putchar(*(char *)arg);
    EndA(&gestore);
    pausetta();
  }
  return 0;
}

void *B(void *arg)
{
  for (;;) {
    StartB(&gestore);
    putchar(*(char *)arg);
    EndB(&gestore);
    pausetta();
  }
  return 0;
}

void *C(void *arg)
{
  for (;;) {
    StartC(&gestore);
    putchar(*(char *)arg);
    EndC(&gestore);
    pausetta();
  }
  return 0;
}

void *D(void *arg)
{
  for (;;) {
    StartD(&gestore);
    putchar(*(char *)arg);
    EndD(&gestore);
    pausetta();
  }
  return 0;
}

void *E(void *arg)
{
  for (;;) {
    StartE(&gestore);
    putchar(*(char *)arg);
    EndE(&gestore);
    pausetta();
  }
  return 0;
}



int main (int argc, char **argv) {
    pthread_attr_t a;
    pthread_t p;
    
    /* inizializzo il mio sistema */
    init_gestore(&gestore);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    //serve per non avere sempre gli stessi numeri nella random
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    //non c'è bisogno di aspettare la fine del thread con la join
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, A, (void *)"a");
    pthread_create(&p, &a, A, (void *)"A");

    pthread_create(&p, &a, B, (void *)"B");
    pthread_create(&p, &a, B, (void *)"b");
    pthread_create(&p, &a, B, (void *)"x");

    pthread_create(&p, &a, C, (void *)"C");
    pthread_create(&p, &a, C, (void *)"c");

    pthread_create(&p, &a, D, (void *)"D");
    pthread_create(&p, &a, D, (void *)"d");

    pthread_create(&p, &a, E, (void *)"E");
    pthread_create(&p, &a, E, (void *)"e");

    pthread_attr_destroy(&a);

    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);

    return 0;
}