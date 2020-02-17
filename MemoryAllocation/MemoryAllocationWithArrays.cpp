#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h> 
#include <queue> 
#include <semaphore.h>
using namespace std;

#define NUM_THREADS 5
#define MEMORY_SIZE 1000

struct node
{
	int id;
	int size;
};


queue<node> myqueue; // shared que
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores

int thread_message[NUM_THREADS]; // thread memory information
char  memory[MEMORY_SIZE]; // memory size
int index=0;



void release_function()
{
	//This function will be called
	//whenever the memory is no longer needed.
	//???????It will kill all the threads ??????????
	while( !myqueue.empty() )
	{
		myqueue.pop();
	}

}

void my_malloc(int thread_id, int size)
{
	node newNode;
	newNode.id=thread_id;
	newNode.size=size;
	pthread_mutex_lock(&sharedLock);	//lock
	myqueue.push(newNode);
	pthread_mutex_unlock(&sharedLock); //unlock
	
}

void * server_function(void *)
{
	while(true){
	//This function should grant or decline a thread depending on memory size.
		pthread_mutex_lock(&sharedLock);	//lock
		if(myqueue.empty())
		{
			pthread_mutex_unlock(&sharedLock); //unlock
			usleep(3000);
		}
		else
		{
			node first=myqueue.front();
	
			if(first.size + index < MEMORY_SIZE)
			{
				thread_message[first.id]=index;
				index+=first.size;
		
			}
			else
				thread_message[first.id]=-1;
		myqueue.pop();
		sem_post(&semlist[first.id]);
		pthread_mutex_unlock(&sharedLock); //unlock
	
		}
	}

}

void * thread_function(void * id) 
{
	//This function will create a random size, and call my_malloc
	int needSize= rand() % (MEMORY_SIZE / 4) +1 ;
	int*  paramPointer = (int *) id;
	int threadId= *paramPointer;

	my_malloc(threadId,needSize);
		
	 sem_wait(&semlist[threadId]);
	 	
	pthread_mutex_lock(&sharedLock);	//lock in case a user want to call that function before joining threads
	 if(thread_message[threadId]==-1)
		 printf("thread %d: Not enough memory" ,threadId);
	 else
	 {
		 for(int i=thread_message[threadId];i<thread_message[threadId] + needSize;i++)
		 {
			 char one = '1'; memory[i] = one;
		 }

	 }
	 	pthread_mutex_unlock(&sharedLock); //unlock

	 //Sleep wake up mý kullancaz ? mymalloc u çaðýrdýktan sonra serverdan gelcek mesajý beklemesi gerekmiyor mu
	//Then fill the memory with 1's or give an error prompt
}

void init()	 
{
	pthread_mutex_lock(&sharedLock);	//lock
	for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
	{sem_init(&semlist[i],0,0);}
	for (int i = 0; i < MEMORY_SIZE; i++)	//initialize memory 
  	{char zero = '0'; memory[i] = zero;}
   	pthread_create(&server,NULL,server_function,NULL); //start server 
	pthread_mutex_unlock(&sharedLock); //unlock
}



void dump_memory()  
{
 // You need to print the whole memory array here.
	pthread_mutex_lock(&sharedLock);	//lock in case a user want to call that function before joining threads
	printf("Memory Dump:\n");
	for(int i=0;i < MEMORY_SIZE; i++)
	{
 		printf("%c" ,memory[i]); // this will print out the memory indexes
	}

	pthread_mutex_unlock(&sharedLock); //unlock


}

int main (int argc, char *argv[])
 {
	 srand(time(NULL));
 	pthread_t thread[NUM_THREADS];
 	init();	// call init
	int threadid[NUM_THREADS];
 	//You need to create threads with using thread ID array, using pthread_create()
	for(int i=0;i < NUM_THREADS; i++)
	{
		threadid[i] = i;
		pthread_create(&thread[i],NULL,thread_function, (void *) &threadid[i]);
	}

 	//You need to join the threads
	for(int i=0;i < NUM_THREADS; i++)
	{
		pthread_join(thread[i],NULL);  
	}

	pthread_cancel(server);
 	dump_memory(); // this will print out the memory
 	printf("\nMemory Indexes:\n" );
 	for (int i = 0; i < NUM_THREADS; i++)
 	{
 		printf("[%d]" ,thread_message[i]); // this will print out the memory indexes
 	}
 	printf("\nTerminating...\n");
 }
