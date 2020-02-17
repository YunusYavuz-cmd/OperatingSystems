#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h> 
#include <queue> 
#include <semaphore.h>
using namespace std;

#define NUM_THREADS 3
#define MEMORY_SIZE 10
/*
* For compilation: g++ -o pthread2.out hw4.cpp -lpthread


	Interactive system where more then 1 reader can read linkedlist(checklinkedlist,printlinkedlist functions are readers) 
*/  	
struct node
{
	int id;
	int size;
	int index;
	node* next;
	node(int id1=-1, int size1=MEMORY_SIZE, int index1=0 ,node *next1=NULL)
	{
		id=id1;
		size=size1;
		index=index1;
		next=next1;
	}
};
void PrintLinkedList();

queue<node> myqueue; // shared queue
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_mutex_t printLock = PTHREAD_MUTEX_INITIALIZER; // mutex to lock print
pthread_mutex_t linkedListLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_mutex_t rcUpdateLock = PTHREAD_MUTEX_INITIALIZER; // mutex to update reader count
pthread_mutex_t memoryArrayLock = PTHREAD_MUTEX_INITIALIZER; // mutex to update reader count
int rc=0;
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores

int thread_message[NUM_THREADS]; // thread memory information
char  memory[MEMORY_SIZE]; // memory size
node* top=NULL;
int threadid[NUM_THREADS];
pthread_t thread[NUM_THREADS];
void CheckAndJoinRight(node* main, node* other)
{	
	if(other->id==-1) // if the other node(right node) is also an empty node, we need to join them
	{
		main->size+=other->size;  //will add main since its the right side
		main->next=other->next;
		delete other;
	}
}
void CheckAndJoinLeft(node* main, node* other) // it will join the main(the thread that do the empty) to the left side, so we dont need to do an additional check on if we change top
{
	if(other->id==-1) // if the other node(left node) is also an empty node, we need to join them
	{
		other->size+=main->size;  //will add main since its the right side
		other->next=main->next;
		delete main;
	}
}

void free_mem(int threadId) // WRITER TO LINKED LIST
{ 

	pthread_mutex_lock(&linkedListLock); //lock linked list
	node* tempBef=NULL;   //we need to trace the node before since we will check both left and right neighbours and its not a double linked list
	node* temp=top;
	bool isFound=false;
	while( !isFound && temp!=NULL ) //checks linked list to find the same threadId in linked list
	{	
		if( temp->id == threadId )
			isFound=true;
		else
		{
			tempBef=temp;		//next node
			temp=temp->next;    //next node
		}
	}
	if(isFound)  //if found the node with same threadId
	{		
		int lastIndex= temp->index + temp->size;  //reads index and size of that thread
		int firstIndex= temp->index;
		temp->id=-1;
		if( temp->next != NULL )				     		 //it can be last node in the LinkedList
		{	CheckAndJoinRight(temp,temp->next);	}
		if(tempBef != NULL) 									// it can be first node in the LinkedList
		{	CheckAndJoinLeft(temp,tempBef);		}
		
		pthread_mutex_unlock(&linkedListLock);

		
		pthread_mutex_lock(&memoryArrayLock);
		for(int i= temp->index; i < lastIndex ;i++)
		{char x='x';memory[i]=x;}
		pthread_mutex_unlock(&memoryArrayLock);
	}
	else
	{
		cout<<"Memory is not in Linked List!Unnecessary function call to empty the thread from the LinkedList";	
		pthread_mutex_unlock(&linkedListLock); 
	}
}
void release_function()
{
	//This function will be called... initial state tek node linked list mi ?
	pthread_mutex_lock(&linkedListLock);
	while( top!=NULL )
	{
		node *temp= top;
		top=top->next;
		delete temp;
	}		exit(1);
	for(int i=0;i < NUM_THREADS; i++)
	{
		pthread_exit(&thread[i]);
	}

	pthread_mutex_unlock(&linkedListLock);

}
bool CheckLinkedList(int threadId)//  //READERS AND WRITERS PROBLEM || READER FROM LINKED LIST
{																							/*IN OUR SITUATION THERE WILL BE READERS AS MUCH AS WRITERS SO WRITERS WON'T STARVE*/
																							//by using this method there can be several readers on linked list
	/*Reader Start*/
	pthread_mutex_lock(&rcUpdateLock);	//lock 
	rc+=1;
	if( rc == 1 ) 
		pthread_mutex_lock(&linkedListLock);
	pthread_mutex_unlock(&rcUpdateLock);	//unlock  access for reader count
	/**************/

	bool result=false;
	node *temp=top;
	while( temp != NULL )
	{
		if( temp->id == threadId )
			result = true;
		temp=temp->next;
	}

	/*Reader End*/
	pthread_mutex_lock(&rcUpdateLock);
	rc-=1;
	if(rc==0)
		pthread_mutex_unlock(&linkedListLock);
	pthread_mutex_unlock(&rcUpdateLock);	//unlock  access for reader count
	/************/
		
	return result;
}
bool my_malloc(int threadId, int size)
{
	node newNode;
	newNode.id=threadId;
	newNode.size=size;
	pthread_mutex_lock(&queueLock);	//lock
	myqueue.push(newNode);
	pthread_mutex_unlock(&queueLock); //unlock
	sem_wait(&semlist[threadId]);

	if( CheckLinkedList(threadId) )
		return true;
	return false;
	
}
void use_mem()
{
	int sleepRand= ( rand()%4 ) +1;
	sleep(sleepRand);
}
bool AllocateMemory(int threadId,int size) //ANOTHER WRITER . 
{//Since there are only one server function no other thread can allocate memory between check(first_fit function calll) and add_mem. Memory can be deallocated but it wont create a problem
	pthread_mutex_lock(&linkedListLock);	 //READERS AND WRITERS PROBLEM || WRITER TO LINKED LIST
	node *temp=top;
	bool isFound=false;
	while( !isFound && temp != NULL)
	{
		if( temp->id == -1 )
		{
				if(temp->size == size) //one node can remain(changing id is enough)
				{
					temp->id = threadId;
					isFound=true;
				}
				else if( temp->size > size )// two nodes needed. 1 will be created
				{
					int size2= temp->size - size;
					int index2= temp->index + size;
					node *newNode= new node(-1,size2,index2,temp->next);
					temp->next=newNode;
					temp->id = threadId;
					temp->size = size;
					isFound=true;
				}
				else
					temp=temp->next;
		}
		else
			temp=temp->next;
	}
	pthread_mutex_unlock(&linkedListLock);
	if(isFound)
	{
		pthread_mutex_lock(&memoryArrayLock);
		char thread = (('0'+threadId)%48)+'0' ;
		int indexArr=temp->index;
		for( int i=indexArr ; i < indexArr+size ; i++ )
		{
			memory[i] = thread;
		}
		pthread_mutex_unlock(&memoryArrayLock);
	}
	return isFound;
}
void PrintLinkedList() //READER LinkedList
{
	/*Reader Start*/
	pthread_mutex_lock(&rcUpdateLock);	//lock 
	rc+=1;	
	if( rc == 1 ) 
		pthread_mutex_lock(&linkedListLock);
	pthread_mutex_unlock(&rcUpdateLock);	//unlock  access for reader count
	/**************/
	node* temp=top;
	printf("List:\n");
	while( temp->next!=NULL )
	{
		printf("[%i][%i][%i]--" ,temp->id,temp->size,temp->index);
		temp=temp->next;
	}
	printf("[%i][%i][%i]" ,temp->id,temp->size,temp->index);
	sleep(3);
	/*Reader End*/
	pthread_mutex_lock(&rcUpdateLock);
	rc-=1;	
	if(rc==0)
		pthread_mutex_unlock(&linkedListLock);
	pthread_mutex_unlock(&rcUpdateLock);	//unlock  access for reader count
	/************/
}

void PrintArray()
{
	pthread_mutex_lock(&memoryArrayLock);
	printf("\nMemory Dump:\n");
	for(int i=0;i < MEMORY_SIZE; i++)
	{
 		printf("%c" ,memory[i]); // this will print out the memory indexes
	}
	printf("\n*********************************\n");
	pthread_mutex_unlock(&memoryArrayLock);
}
void dump_memory()  
{
	pthread_mutex_lock(&printLock);
	PrintLinkedList();
	PrintArray();
	pthread_mutex_unlock(&printLock);
}
void * server_function(void *)
{
	while(true){
	//This function should grant or decline a thread depending on memory size.
		pthread_mutex_lock(&queueLock);	//lock
		if(myqueue.empty())
		{
			pthread_mutex_unlock(&queueLock); //unlock
			usleep(500);
		}
		else
		{
			node first=myqueue.front();
			if(AllocateMemory(first.id,first.size));
			{
				dump_memory();		
			}
		sem_post(&semlist[first.id]);
		myqueue.pop();
		pthread_mutex_unlock(&queueLock); //unlock
	
		}
	}

}

void * thread_function(void * id) 	
{
	int*  paramPointer = (int *) id;
	int threadId= *paramPointer;
	while(true){
	//This function will create a random size, and call my_malloc
	int needSize=rand() % (MEMORY_SIZE / 3)/*to not have mod 0*/  +  1/*to not have size 0 +1*/; 
	
		if(my_malloc(threadId,needSize))
		{
			use_mem();
			free_mem(threadId);
		}
	}
}

void init()	 
{
	for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
	{sem_init(&semlist[i],0,0);}
	for (int i = 0; i < MEMORY_SIZE; i++)	//initialize memory 
  	{memory[i] = 'x';}
	top= new node(-1,MEMORY_SIZE,0,NULL);
   	pthread_create(&server,NULL,server_function,NULL); //start server 

}


int main (int argc, char *argv[])
 {
	 srand(time(NULL)); // to have better randomness

 	init();	// call init

 	//You need to create threads with using thread ID array, using pthread_create()
	for(int i=0;i < NUM_THREADS; i++)
	{
		threadid[i] = i;
		pthread_create(&thread[i],NULL,thread_function, (void *) &threadid[i]);
	}
		
	sleep(10);
	release_function();
	

 }