/*
* For compilation: g++ -o pthread2.out pthread2.cpp -lpthread
*/

#include <iostream>
#include <pthread.h>
#include <cstdlib>
using namespace std;

int seatArray[2][50]={0};
int turn=0;

void *Reserve_Function( void *param )
{

	while(true)
	{
		int*  paramPointer = (int *) param;
		int threadId= *paramPointer;
		int reserveSeat= rand() %50 ;
		int x=rand()%2;
		int y=reserveSeat;
		while(turn != threadId-1);
		//start Critical Region
		cout<<"Thread no: "<<threadId<<" entered Critical Region"<<endl;
		if(seatArray[x][y]==0)
	 	{
				
			seatArray[x][y]= threadId ;
	 	}
		turn= (turn+1)%3;
		//end Critical Region
	}
}

int main()
{
	pthread_t  TravelAgency1, TravelAgency2, TravelAgency3;
	int id1 = 1;
	int id2 = 2; 
	int id3 = 3; 

	cout<<"Started \n";
	pthread_create( &TravelAgency1, NULL, Reserve_Function , &id1);
	pthread_create( &TravelAgency2, NULL, Reserve_Function , &id2);
	pthread_create( &TravelAgency3, NULL, Reserve_Function , &id3);
	cout<<"Threads Created \n";
	bool checkMatrix=true;
	bool zeroFound = false;
	while(checkMatrix)
	{
		
		for(int i=0;i<2 && checkMatrix;i++){
			for(int a=0; a<50 ;a++)
			{
				if(seatArray[i][a]==0){
					zeroFound=true;
					}
			

			}	
		}
			if(!zeroFound)
				checkMatrix=false;
			else
				zeroFound=false;
		
	}
	pthread_cancel(TravelAgency1);	
	pthread_cancel(TravelAgency2);
	pthread_cancel(TravelAgency3);

	for(int i=0;i<50;i++){
			for(int a=0; a<2 ;a++)
			{
				cout<<seatArray[a][i]<< "	" ;
			}
				cout<<endl;

	}

	return 0;
}
