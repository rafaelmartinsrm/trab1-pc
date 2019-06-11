#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define DETECTIVES_N 10
#define CLIENTS_N 2
#define CRIME_PROBABILITY 10
#define LOCKDOWN_PROBABILITY 100

typedef struct _thread_data_t
{
  int tid;
} thread_data_t;


pthread_mutex_t mutex;
pthread_cond_t interview = PTHREAD_COND_INITIALIZER;
pthread_cond_t crime = PTHREAD_COND_INITIALIZER;
pthread_barrier_t lockdown_barrier;
pthread_barrier_t lockdown_barrier_after;
thread_data_t thr_data[DETECTIVES_N+CLIENTS_N];

int lockdown = 0;
int detectives = 0;
int clients = 0;
int interviewed = 0;
int crimes = 0;
int stopIssued = 0;

void * detective(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int i;

    pthread_mutex_lock(&mutex);
    printf("[DETETIVE] Um detetive chegou #%d.\n", data->tid);
    detectives++;
    pthread_mutex_unlock(&mutex);

    while(1)
    {
        pthread_mutex_lock(&mutex);
        while(crimes == 0)
        {
            pthread_cond_wait(&crime, &mutex);
        }
	if(stopIssued)
	{
		pthread_mutex_unlock(&mutex);
		break;
	}
        crimes--;
        detectives--;

        printf("[STATUS] O detetive #%d está interrogando. Esperar 10 minutos.\n", data->tid);
        pthread_mutex_unlock(&mutex);

        sleep(10);

        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&interview); //AVISAR QUE INTERROGOU
        printf("[STATUS] O detetive #%d está disponível novamente.\n", data->tid);
        detectives++;
        interviewed++;
        pthread_mutex_unlock(&mutex);
        sleep(2 + rand() % 4);
    }
    printf("[DETETIVE] O detetive #%d volta para o departamento.\n", data->tid);

}

void * client(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int probability = 0;
    int cur_crime = 0;

    while(1)
    {
        pthread_mutex_lock(&mutex);

        if(lockdown == 1)
        { 
	    crimes++;
            pthread_mutex_unlock(&mutex);
            pthread_barrier_wait(&lockdown_barrier);
            pthread_mutex_lock(&mutex);
            printf("[CLIENTE] O cliente #%d foi enviado para inquérito por conta do lockdown.\n", data->tid);
            pthread_cond_signal(&crime);
	    clients--;
            while(interviewed == 0)
            {
                pthread_cond_wait(&interview, &mutex);
            }
            printf("[CLIENTE] O cliente #%d foi para casa após ser interrogado.\n", data->tid);
            interviewed--;
	    pthread_mutex_unlock(&mutex);
	    pthread_barrier_wait(&lockdown_barrier_after);
	    pthread_mutex_lock(&mutex);
	    if(stopIssued == 0)
	    {
                stopIssued = 1;
     	        printf("[CLIENTE] Todos foram para casa.\n", data->tid);
            }
	    if(stopIssued)
	    {
		crimes = CLIENTS_N;
		pthread_cond_broadcast(&crime);
            }
	    pthread_mutex_unlock(&mutex);
	    break;
        }

        probability = rand() % 100;
        if(probability > 100-CRIME_PROBABILITY)
        {
            cur_crime = 0;
        }
        else
        {
            probability = rand() % 100;
            cur_crime = 1;
            if(detectives > 0 && probability > 100-LOCKDOWN_PROBABILITY)
            {
                cur_crime = 2;
            }
        }

        switch(cur_crime)
        {
            case 0:
                // printf("[CLIENTE] O cliente #%d está dançando.\n", data->tid);
                break;
            case 1:
                if(detectives == 0)
                {
                    printf("[CRIME] O cliente #%d cometeu um crime mas ninguém viu.\n", data->tid);
                }
                else
                {
                    printf("[CLIENTE] O cliente #%d foi flagrado em um crime e será levado para inquérito.\n", data->tid);
                    crimes++;
                    pthread_cond_signal(&crime);
                    clients--;
                    while(interviewed == 0)
                    {
                        pthread_cond_wait(&interview, &mutex);
                    }
                    printf("[CLIENTE] O cliente #%d retornou após ser interrogado.\n", data->tid);
                    clients++;
                    interviewed--;
                }
                break;
            case 2:
                if(detectives == 0)
                {
                    printf("[CRIME] O cliente #%d cometeu um crime mas ninguém viu.\n", data->tid);
                }
                else
                {
                    lockdown = 1;
                    printf("[LOCKDOWN] Polícia para festa por conta de um crime.\n");
                    pthread_barrier_init(&lockdown_barrier, NULL, CLIENTS_N);
                    pthread_barrier_init(&lockdown_barrier_after, NULL, CLIENTS_N);
                }
                break;
        }
        pthread_mutex_unlock(&mutex);
        sleep(2 + rand() % 3);
    }
}



int main()
{
    int i = 0;
    int clients_counter = 0;
    int detectives_counter = 0;
    pthread_t thr[DETECTIVES_N+CLIENTS_N];
    clients = CLIENTS_N;

    srand(time(0));

    for(i = 0; i < DETECTIVES_N; ++i)
    {
        pthread_create(&thr[i], NULL, &detective, &thr_data[i]);
        thr_data[i].tid = detectives_counter;
        detectives_counter++;
        sleep(1);
    }

    for(i = DETECTIVES_N; i < CLIENTS_N+DETECTIVES_N; ++i)
    {
        pthread_create(&thr[i], NULL, &client, &thr_data[i]);
        thr_data[i].tid = clients_counter;
        clients_counter++;
        sleep(1);
    }

    for(i = 0; i < DETECTIVES_N+CLIENTS_N; ++i)
    {
        pthread_join(thr[i], NULL);
    }

    pthread_exit(NULL);

    return 1;
}
