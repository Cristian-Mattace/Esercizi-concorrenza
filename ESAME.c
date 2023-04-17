/*
ESAME REAL-TIME EMBEDDED SYSTEMS 
CRISTIAN MATTACE MATR.179010

A causa di una svista durante la lettura della consegna all'esame,
su carta non è stato implementato l'accesso alle macchine in 
ordine di arrivo.

Nella soluzione seguente, rispetto a quella implementata su carta,
è stata inserita anche la coda FIFO sulla quale i processi si 
sospendono in attesa dell'attrezzo di una determinata tipologia.

Inoltre, nella soluzione su carta, non avendo inserito la coda 
FIFO, non si poneva il seguente problema:
    Se un thread libera un attrezzo eseguendo una post sul semaforo
    corrispondente, non è detto che sia il primo thread sospeso a
    continuare la sua esecuzione, perchè può capitare che un altro 
    thread prenda al suo posto il mutex continuando l'esecuzione.

Nella seguente soluzione per risolvere il precedente problema ho
utilizzato la tecnica del token passing.
Sostanialmente un processo che termina la sua esecuzione su un 
tipo di attrezzo, controlla la coda dei processi bloccati su quel
tipo di attrezzo, e se è presente qualcuno, allorà eseguirà la post
sulla coda, ma non sul semaforo dell'attrezzo, eseguendo così il
token passing.

Le parti commentate nel codice sono principalmente parti aggiunte
dopo la consegna del codice cartaceo.

*/

#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define N 3
#define M 2
#define P 10

typedef enum {false, true} Boolean;

struct palestra_t{
    sem_t mutex;
    sem_t semAttrezzo[N];

    sem_t semSospesi[N][P-M];   //matrice Nx(P-M) di semafori dove si sospendono i processi che rimangono in attesa dell'attrezzo
    int head[N];                //vettore di N che contiene la posizione del primo elemento da svegliare
    int tail[N];                //vettore di N che contiene la posizione dell'ultimo elemento da svegliare
    int sospesi[N];             //vettore di N elementi che indica il numero di processi sospesi per ogni tipo di attrezzo

    int prenotazioni[N][M];

    int liberi[N];

    int inUso[N];               //vettore di N elementi utile per sapere quali tipi di risorse e quante entità sono in utilizzo
    
} palestra;


void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0; 
    t.tv_nsec = (rand()%10+1)*1000000;
    nanosleep(&t,NULL);
}


void init_palestra(struct palestra_t *p){
    sem_init(&p->mutex, 0, 1);

    for(int i=0; i<N; i++){
        sem_init(&p->semAttrezzo[i], 0, M);
        p->liberi[i] = M;

        //inizializzo i vettori a 0
        p->sospesi[i] = 0;
        p->head[i] = 0;
        p->tail[i] = 0;
        p->inUso[i] = 0;

        //inizializzo la matrice di prenotazioni tutta a -1, come valore di default
        for(int v=0; v<M; v++){
            p->prenotazioni[i][v] = -1;
        }

        //inizializzo la matrice di semafori a 0, così ogni wait è bloccante
        for(int z=0; z<P-M; z++){
            sem_init(&p->semSospesi[i][z], 0, 0);
        }
    }
}


//funzione implementata solo per poter visualizzare lo stato delle risorse durante l'esecuzione
void stato(struct palestra_t *p){

    int v;
    printf("\nPosti liberi:   [ ");
    for(int x=0; x<N; x++){
        sem_getvalue(&p->semAttrezzo[x], &v);
        if(x<N-1) printf("[ %d - %d ], ", p->liberi[x], v);
        else printf("[ %d - %d ]", p->liberi[x], v);
    }
    printf(" ]");


    printf("\nPrenotazioni:   [ ");
    for(int x=0; x<N; x++){
        printf("[");
        for(int y=0; y<M; y++){
            if(y<M-1) printf(" %d, ", p->prenotazioni[x][y]);
            else printf("%d", p->prenotazioni[x][y]);
        }
        if(x<N-1) printf(" ], ");
        else printf(" ]");
    }
    printf(" ]");

    printf("\nSospesi:   [ ");
    for(int x=0; x<N; x++){
        if(x<N-1) printf("[ %d ], ", p->sospesi[x]);
        else printf("[ %d ]", p->sospesi[x]);
    }
    printf(" ]");

    printf("\nIN USO:  [ ");
    for(int x=0; x<N; x++){
        if(x<N-1) printf("[ %d ], ", p->inUso[x]);
        else printf("[ %d ]", p->inUso[x]);
    }
    printf(" ]");
    

    printf("\n\n");
}


void usaattrezzo(struct palestra_t *p, int numeropersona, int tipoattrezzo){
    Boolean uso = false;

    sem_wait(&p->mutex);

    printf("Sono il thread [ %d ] e voglio usare l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);

    for(int cnt=0; cnt<M; cnt++){
        if(p->prenotazioni[tipoattrezzo][cnt] == numeropersona){
            printf("Sono il thread [ %d ] ero prenotato e sto usando l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);
            p->prenotazioni[tipoattrezzo][cnt] = -1;
            uso = true;

            p->inUso[tipoattrezzo]++; //incremento di uno il numerod i attrezzi di quel tipo in uso
            stato(p);   //stampa lo stato corrente
            break;
        }
    }
    
    sem_post(&p->mutex);

    if(!uso){
        printf("Sono il thread [ %d ], non ho prenotato e provo a prendere l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);

        sem_wait(&p->mutex);

        //prendo il valore del semaforo per un tipo di attrezzo
        int v;
        sem_getvalue(&p->semAttrezzo[tipoattrezzo], &v);

        //se il valore del semaforo è 0, oppure se ci sono processi sospesi per quel tipo di attrezzo, allora bisogna accodare il processo
        if(p->sospesi[tipoattrezzo] > 0 || v == 0){
            
            printf("Sono il thread [ %d ], non avevo prenotato, non ci sono tipi [ %d ] disponibili, mi metto in coda\n", numeropersona, tipoattrezzo);

            //prendo la posizione di coda del tipo corrispondente
            int posizione = p->tail[tipoattrezzo];
            //modifico la posizione di coda del tipo corrispondente con il modulo della grandezza massima
            //così diventa una coda circolare FIFO e non sfora l'indice massimo 
            p->tail[tipoattrezzo] = (p->tail[tipoattrezzo] + 1) % (P-M);

            //incremento il numero di processi sospesi e rilascio il mutex
            p->sospesi[tipoattrezzo]++;
            sem_post(&p->mutex);

            //sospendo il processo sulla coda FIFO, sul tipo corrispondente e nell'ultima posizione
            sem_wait(&p->semSospesi[tipoattrezzo][posizione]);

            //una volta arrivato qui, non ha bisogno di fare la wait sul semaforo dell'attrezzo, perchè si usa la tecnica del TOKEN PASSING

            printf("Sono il thread [ %d ] e mi hanno SVEGLIATO per il tipo [ %d ]\n", numeropersona, tipoattrezzo);

            sem_wait(&p->mutex);
            p->sospesi[tipoattrezzo]--;
        }
        else{
            sem_wait(&p->semAttrezzo[tipoattrezzo]);
            p->liberi[tipoattrezzo]--;
        }

        printf("Sono il thread [ %d ] e sto usando l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);
        p->inUso[tipoattrezzo]++;
        stato(p);
        sem_post(&p->mutex);
    }
}


void prenota(struct palestra_t *p, int numeropersona, int tipoattrezzo){
    int v;

    sem_wait(&p->mutex);

    //prendo il valore del semaforo dell'attrezzo del tipo corrispondente
    sem_getvalue(&p->semAttrezzo[tipoattrezzo], &v);
    //if(p->liberi[tipoattrezzo] > 0){
    //controllo il valore del semaforo, perchè il valore di p->liberi[tipoattrezzo] potrebbe non essere ancora stato modificato dato che il processo 
    //potrebbe non aver ancora preso subito il MUTEX
    if(v>0){    
        for(int cnt=0; cnt<M; cnt++){
            if(p->prenotazioni[tipoattrezzo][cnt] == -1){
                p->prenotazioni[tipoattrezzo][cnt] = numeropersona;
                sem_wait(&p->semAttrezzo[tipoattrezzo]);
                p->liberi[tipoattrezzo]--;
                printf("Sono il thread [ %d ] ed ho prenotato l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);
                break;
            }
        }
    }
    stato(p);
    sem_post(&p->mutex);
}


void fineuso(struct palestra_t *p, int numeropersona, int tipoattrezzo){
    sem_wait(&p->mutex);

    //se ci sono processi sospesi, allora dovrò fare una post sulla matrice di semafori
    if(p->sospesi[tipoattrezzo] > 0){
        //la post viene fatta sul tipo corrispondente, e sull'indice di testa
        sem_post(&p->semSospesi[tipoattrezzo][p->head[tipoattrezzo]]);
        //aggiorno l'indice di testa sempre con il modulo della grandezza massimo per la coda FIFO
        p->head[tipoattrezzo] = (p->head[tipoattrezzo] + 1) % (P-M);
        printf("Sono il thread [ %d ] ed ho svegliato qualcuno sul tipo [ %d ]\n", numeropersona, tipoattrezzo);
        //una volta svegliato il processo, non si esegue la post sul semaforo corrispondente perchè si usa la tecnica del TOKEN PASSING
    }
    else{
        //se invece non c'è nessun processo sospeso sulla coda FIFO, allora si fa la POST sul semaforo dell'attrezzo corrispondente
        sem_post(&p->semAttrezzo[tipoattrezzo]);    
        p->liberi[tipoattrezzo]++;
    }

    printf("Sono il thread [ %d ] ed ho rilasciato l'attrezzo di tipo [ %d ]\n", numeropersona, tipoattrezzo);
    p->inUso[tipoattrezzo]--;
    stato(p);

    sem_post(&p->mutex);

}


void *persona(void *arg){
    int attrezzocorrente = rand() % N;
    int prossimoattrezzo = rand() % N;
    int E = rand() % 5 + 1;

    int numeropersona = (int) arg;
    for(int i=E; i>0; i--){
        printf("Sono il thread [ %d ] e sto partendo [ %d ]\n", numeropersona, i);
        usaattrezzo(&palestra, numeropersona, attrezzocorrente);
        if(i!=1) prenota(&palestra, numeropersona, prossimoattrezzo);
        pausetta();
        fineuso(&palestra, numeropersona, attrezzocorrente);

        if(i!=1){
            attrezzocorrente = prossimoattrezzo;
            prossimoattrezzo = rand() % N;
        }
    }

    return 0;
}


int main (int argc, char **argv) {
    pthread_attr_t threadAttr;
    pthread_t thread[P];
    int i;

    // init struct
    init_palestra(&palestra);
    // init random numbers
    srand(555);

    pthread_attr_init(&threadAttr);

    
    for(i = 0; i<P; i++)
        pthread_create(&thread[i], &threadAttr, persona, (void *) i);


    pthread_attr_destroy(&threadAttr);

    pausetta();
    for(i = 0; i < P; i++)
        pthread_join(thread[i], NULL);

    return 0;
}