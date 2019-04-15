#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define QUANTITY 100000
#define BUFFER_SIZE 1
#define RANDOM_LIMIT 10000000
#define AVERAGE_ROUNDS 1


typedef enum { false, true } bool;

sem_t mutex, empty, full;
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
    for(int i = 3; i < num / 2; i+= 2){
        if (num % i == 0)
            return false;
    }
    return true;
}


void *producerThread(void *arg){
	int id = *((int *)arg);
	int number, position = 0;
	bool active = true;
	//printf("%d\n", produced);

	while(active){
		sem_wait(&empty);
		sem_wait(&mutex);

		if(produced > 0){
			position = getEmptyPosition(shared_memory);
			if(position >= 0){
				number = rand() % RANDOM_LIMIT;
				shared_memory[position] = number;
			}
			//printf("produzido - %i\n", number);
			--produced;
		} else {
			active = false;
		}

		sem_post(&mutex);
		sem_post(&full);
	}
	printf("out\n");
}

void *consumerThread(void *arg){
	int number = -1;
	int position = 0;
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
			//bool prime = isPrime(number); 
			//printf("%i %sé primo\n", consumed, prime?"":"não ");
			--consumed;
		} else {

			active = false;
		}

		sem_post(&mutex);
		sem_post(&empty);	

		if(number >= 0){
			bool prime = isPrime(number); 
			//printf("%i %sé primo\n", number, prime?"":"não ");
			number = -1;
		}
	}
	printf("fora\n");
}

int createProducerConsumerThreads(int nProd, int nCom){
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
	return 0;
}

int main(){
	sem_init(&mutex, 0, 1);
	sem_init(&empty, 0, BUFFER_SIZE);
	sem_init(&full, 0, 0);


	for(int i=0; i < BUFFER_SIZE; i++){
		shared_memory[i] = 0;
	}

	srand(time(NULL));

	clock_t begin[9];
	clock_t end[9];
	int nProd[9] = {1, 1, 1, 1, 1, 2, 4, 8, 16};
	int nCom[9] = {1, 2, 4, 8, 16, 1, 1, 1, 1};

	for(int i = 0; i < 9; i++){
		begin[i] = 0;
		end[i] = 0;
	}

	for(int i = 0; i < 9; i++){
		printf("a\n");
		for(int j = 0; j < AVERAGE_ROUNDS; j++){
			begin[i] += clock();
			createProducerConsumerThreads(2, 1);
			end[i] += clock();
		}
		printf("%d\n", i);
	}	

	for(int i=0; i < 9; i++){
		double avg_diff_t = ((double)(end[i] - begin[i]))/(CLOCKS_PER_SEC * AVERAGE_ROUNDS);
		printf("tempo combo %d: %f\n", i, avg_diff_t);
	}


	sem_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);
	return 0;
}