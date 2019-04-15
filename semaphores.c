#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define N_PRODUCERS 2
#define N_CONSUMERS 2
#define QUANTITY 10
#define BUFFER_SIZE 5

typedef enum { false, true } bool;

sem_t mutex, empty, full;
int shared_memory[BUFFER_SIZE];

int getEmptyPosition(int array[]){
	for(int i = 0; i < BUFFER_SIZE; i++){
		if(array[i] == 0)
			return i;
	}
	return -1;
}

int getFullPosition(int array[]){
	for(int i = 0; i < BUFFER_SIZE; i++){
		if(array[i] != 0)
			return i;
	}
	return -1;
}

bool isPrime(int num){
    if (num <= 1) return false;
    if (num % 2 == 0 && num > 2) return false;
    for(int i = 3; i < num / 2; i+= 2){
        if (num % i == 0)
            return false;
    }
    return true;
}


void *producerThread(void *arg){
	static int produced = QUANTITY;
	int id = *((int *)arg);
	int number, position = 0;
	bool active = true;

	while(active){
		sem_wait(&empty);
		sem_wait(&mutex);

		if(produced > 0){
			position = getEmptyPosition(shared_memory);
			if(position >= 0){
				number = rand() % 501;
				shared_memory[position] = number;
			}

			printf("produzido - %i\n", number);
			--produced;
		} else {
			active = false;
		}

		sem_post(&mutex);
		sem_post(&full);

		//printf("produzido - %i\n", number);
	}
}

void *consumerThread(void *arg){
	static int consumed = QUANTITY;
	int number, position = 0;
	bool active = true;

	while(active){
		sem_wait(&full);
		sem_wait(&mutex);

		if(consumed > 0){
			position = getFullPosition(shared_memory);
			if(position >= 0){
				number = shared_memory[position];
				shared_memory[position] = 0;
			}
			bool prime = isPrime(number); 
			printf("%i %sé primo\n", number, prime?"":"não ");
			--consumed;
		} else {

			active = false;
		}

		sem_post(&mutex);
		sem_post(&empty);	

		//printf("consumido - %i\n", number);
	}
}

int main(){
	sem_init(&mutex, 0, 1);
	sem_init(&empty, 0, BUFFER_SIZE);
	sem_init(&full, 0, 0);

	for(int i=0; i < BUFFER_SIZE; i++){
		shared_memory[i] = 0;
	}

	srand(time(NULL));

	pthread_t thread_c_id[N_CONSUMERS];
	pthread_t thread_p_id[N_PRODUCERS];

	for(int i = 0; i < N_PRODUCERS; i++){
		int *arg = malloc(sizeof(*arg));
		*arg = i;

		if(pthread_create(&thread_p_id[i], NULL, producerThread, arg)){
			fprintf(stderr, "error creating thread\n");
			return 1;
		}
	}
	for(int i = 0; i < N_CONSUMERS; i++){
		if(pthread_create(&thread_c_id[i], NULL, consumerThread, NULL)){
			fprintf(stderr, "error creating thread\n");
			return 1;
		}
	}

	for(int i = 0; i < N_PRODUCERS; i++){
		pthread_join(thread_p_id[i], NULL);
	}
	for(int i = 0; i < N_CONSUMERS; i++){
		pthread_join(thread_c_id[i], NULL);
	}

	sem_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);

	return 0;
}