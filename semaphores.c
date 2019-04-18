#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

#define QUANTITY 100000
#define BUFFER_SIZE 1
#define RANDOM_LIMIT 10000000
#define AVERAGE_ROUNDS 1


typedef enum { false, true } bool;

sem_t mutex, empty, full, producing, consuming;
int shared_memory[BUFFER_SIZE];
int consumed = QUANTITY;
int produced = QUANTITY;

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
    
    // Loop otimizado
    for(int i = 3; i < sqrt(num); i+= 2){			
    // Loop ineficiente
    //for(int i = 3; i < num/2; i+= 2){
        if (num % i == 0)
            return false;
    }
    return true;
}


void *producerThread(void *arg){
	int id = *((int *)arg);
	int number = 0;
	bool active = true;

	while(active){
		number = rand() % RANDOM_LIMIT;
		sem_wait(&producing);
		if(produced > 0){
			sem_wait(&empty);
			sem_wait(&mutex);

			int position = getEmptyPosition(shared_memory);
			if(position >= 0){
				shared_memory[position] = number;
			}
			--produced;

			sem_post(&mutex);
			sem_post(&full);
		} else {
			active = false;
		}

		sem_post(&producing);
	}
}

void *consumerThread(void *arg){
	int number = -1;
	bool active = true;

	while(active){
		sem_wait(&consuming);

		if(consumed > 0){
			sem_wait(&full);
			sem_wait(&mutex);

			int position = getFullPosition(shared_memory);
			if(position >= 0){
				number = shared_memory[position];
				shared_memory[position] = 0;
			}
			--consumed;
			
			sem_post(&mutex);
			sem_post(&empty);
		} else {

			active = false;
		}

		sem_post(&consuming);	
		if(number >= 0){
			bool prime = isPrime(number); 
			//printf("%i %sé primo\n", number, prime?"":"não ");
			number = -1;
		}
	}
}

int createProducerConsumerThreads(int nProd, int nCom){
	sem_init(&mutex, 0, 1);
	sem_init(&producing, 0, 1);
	sem_init(&consuming, 0, 1);
	sem_init(&empty, 0, BUFFER_SIZE);
	sem_init(&full, 0, 0);

	pthread_t *thread_c_id = malloc(sizeof(pthread_t) * nCom);
	pthread_t *thread_p_id = malloc(sizeof(pthread_t) * nProd);

	produced = QUANTITY;
	consumed = QUANTITY;

	for(int i = 0; i < nProd; i++){
		int *arg = malloc(sizeof(*arg));
		*arg = i;

		if(pthread_create(&thread_p_id[i], NULL, producerThread, arg)){
			fprintf(stderr, "error creating thread\n");
			free(thread_c_id);
			free(thread_p_id);
			return 1;
		}
	}
	for(int i = 0; i < nCom; i++){
		if(pthread_create(&thread_c_id[i], NULL, consumerThread, NULL)){
			fprintf(stderr, "error creating thread\n");
			free(thread_c_id);
			free(thread_p_id);
			return 1;
		}
	}

	for(int i = 0; i < nProd; i++){
		pthread_join(thread_p_id[i], NULL);
	}
	for(int i = 0; i < nCom; i++){
		pthread_join(thread_c_id[i], NULL);
	}

	free(thread_c_id);
	free(thread_p_id);

	sem_destroy(&mutex);
	sem_destroy(&producing);
	sem_destroy(&consuming);
	sem_destroy(&full);
	sem_destroy(&empty);
	return 0;
}

int main(){
	for(int i=0; i < BUFFER_SIZE; i++){
		shared_memory[i] = 0;
	}

	srand(time(NULL));

	struct timespec begin[9];
	struct timespec end[9];
	int nProd[9] = {1, 1, 1, 1, 1, 2, 4, 8, 16};
	int nCom[9] = {1, 2, 4, 8, 16, 1, 1, 1, 1};

	// for(int i = 0; i < 9; i++){
	// 	begin[i] = 0;
	// 	end[i] = 0;
	// }

	for(int i = 0; i < 9; i++){
		printf("%d\n", i);
		for(int j = 0; j < AVERAGE_ROUNDS; j++){
			clock_gettime(CLOCK_MONOTONIC, &begin[i]);
			createProducerConsumerThreads(nProd[i], nCom[i]);
			clock_gettime(CLOCK_MONOTONIC, &end[i]);
		}
	}	

	for(int i=0; i < 9; i++){
		double avg_diff_t = ( (end[i].tv_sec - begin[i].tv_sec) + ((end[i].tv_nsec - begin[i].tv_nsec) / 1000000000.0) ) / AVERAGE_ROUNDS;
		printf("tempo combo %d: %f\n", i, avg_diff_t);
	}

	return 0;
}