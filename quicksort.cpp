#include <iostream>
#include <list>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <signal.h>

using namespace std;

//constants--------------------------------------------------------------
const int ARR_SIZE = 100;
const int MAX_RAND = 1000;

//job struct-------------------------------------------------------------
struct Job
{
	int left;
	int right;
};

//gobal variables--------------------------------------------------------
pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t job_cond = PTHREAD_COND_INITIALIZER;
list<Job> task_queue;

int arr[ARR_SIZE];

int still_working = 0;
pthread_cond_t finish_cond;
pthread_mutex_t finish_mutex;


//function prototypes----------------------------------------------------
void* sort(void*);
void signalHandler(int arg){}

//main-------------------------------------------------------------------
int main(int argc, char* argv[])
{
	//check arguments
	if (argc !=2)
	{
		cerr<<"Invalid number of arguments\nUSAGE: ./quicksort [# of threads]\n";
		exit(-1);
	}
	
	//get number of threads to make
	int thread_num = atoi(argv[1]);
	
	//fill the array with random numbers and display it
	cout<<"\nUnsorted array: ";
	srand(time(NULL));
	for (int i=0; i<ARR_SIZE;++i)
	{
		arr[i] = rand()%MAX_RAND;
		cout<<arr[i]<<' ';
	}
	cout<<"\n\n";
	
	//add first job to queue
	Job first_job = {0,ARR_SIZE-1};
	task_queue.push_front(first_job);
	
	//create threads
	still_working = thread_num;
	pthread_t* threads = new pthread_t[thread_num];
	for (int i=0;i<thread_num;++i)
	{
		if (pthread_create(&threads[i],NULL,&sort,NULL)<0)
		{
			cerr<<"pthread_create\n";
			exit(-1);
		}
	}

	
	//lock the finished variable
	if(pthread_mutex_lock(&finish_mutex)<0)
	{
		cerr<<"finish lock\n";
	}	
	
	
	//wait until finished
	while (still_working !=0)
	{
		if (pthread_cond_wait(&finish_cond,&finish_mutex)<0)
		{
			cerr<<"pthread fish cond wait\n";
		}
		cout<<"threads still working: "<<still_working<<endl;
	}
	if(pthread_mutex_unlock(&finish_mutex)<0)
	{
		cerr<<"pthread finish unlock\n";
	}
	
	//overide signal handler
	signal(SIGINT,signalHandler);	
	
	//kill theads
	for (int i=0;i<thread_num;++i)
	{
		pthread_kill(threads[i],SIGINT);
	}
	
	//display result
	cout<<"\nSorted array: ";
	for (int i=0;i<ARR_SIZE;++i)
	{
		cout<<arr[i]<<' ';
	}
	cout<<"\n\n";
	
	return 0;
}

//sort function----------------------------------------------------------
void* sort(void* arg)
{
	Job work, new1, new2;
	int i, j, temp, pivot;
	
	while(true)
	{		
		//lock the task queue
		if(pthread_mutex_lock(&job_mutex)<0)
		{
			cerr<<"job lock\n";
			pthread_exit(NULL);
		}
		//go to sleep if task queue is empty
		while(task_queue.empty())
		{
			//thread is going to sleep
			//lock the finish condition
			if(pthread_mutex_lock(&finish_mutex)<0)
			{
				cerr<<"finish lock\n";
				pthread_exit(NULL);
			}
			
			//decrement still working
			still_working--;
			
			//singal main to check if finished
			if(pthread_cond_signal(&finish_cond)<0)
			{
				cerr<<"finish signal\n";
				pthread_exit(NULL);
			}
			
			//unlock the finish condition
			if(pthread_mutex_unlock(&finish_mutex)<0)
			{
				cerr<<"finish unlock\n";
				pthread_exit(NULL);
			}
			
			//wait for more work
			if(pthread_cond_wait(&job_cond,&job_mutex)<0)
			{
				cerr<<"job wait\n";
				pthread_exit(NULL);
			}
			
			//thread is working again
			//lock the finish condition
			if(pthread_mutex_lock(&finish_mutex)<0)
			{
				cerr<<"finish lock\n";
				pthread_exit(NULL);
			}
			
			//increment still working
			still_working++;
			
			//unlock the finish condition
			if(pthread_mutex_unlock(&finish_mutex)<0)
			{
				cerr<<"finish unlock\n";
				pthread_exit(NULL);
			}			
		}
		
		//get a job from the queue
		work = task_queue.back();
		task_queue.pop_back();
		
		//unlock the task queue
		if(pthread_mutex_unlock(&job_mutex)<0)
		{
			cerr<<"job unlock\n";
				pthread_exit(NULL);
		}
		
		//sort
		i = work.left;
		j = work.right;
		pivot = arr[(i+j)/2];
		
		while (i<=j)
		{
			while (arr[i] < pivot)
				i++;
			while (arr[j] > pivot)
				j--;
			if (i <=j )
			{
				temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
				i++;
				j--;
			}
		}
		
		//add work if not done
		if (work.left < j)
		{
			new1.left = work.left;
			new1.right = j;
			
			//lock the job queue
			if(pthread_mutex_lock(&job_mutex)<0)
			{
				cerr<<"job lock\n";
				pthread_exit(NULL);
			}
			
			//add new work to queue
			task_queue.push_front(new1);
			
			//singal that there is more work
			if(pthread_cond_signal(&job_cond)<0)
			{
				cerr<<"job signal\n";
				pthread_exit(NULL);
			}
			
			//unlock the job queue
			if(pthread_mutex_unlock(&job_mutex)<0)
			{
				cerr<<"job unlock\n";
				pthread_exit(NULL);
			}
		}
		if (i < work.right)
		{
			new2.left = i;
			new2.right = work.right;
			
			//lock the job queue
			if(pthread_mutex_lock(&job_mutex)<0)
			{
				cerr<<"job lock\n";
				pthread_exit(NULL);
			}
			
			//add new work to queue
			task_queue.push_front(new2);
			
			//singal that there is more work
			if(pthread_cond_signal(&job_cond)<0)
			{
				cerr<<"job signal\n";
				pthread_exit(NULL);
			}
			
			//unlock the job queue
			if(pthread_mutex_unlock(&job_mutex)<0)
			{
				cerr<<"job unlock\n";
				pthread_exit(NULL);
			}
		}
	}
}


