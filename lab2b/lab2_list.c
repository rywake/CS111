/*                                                                                                                         
NAME: Ryan Wakefield
EMAIL: ryanwakefield@g.ucla.edu
ID: 904975422
*/
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include "SortedList.h"
#include <signal.h>
#include <unistd.h>

#define SIZE_OF_KEY 128

//GLOBAL VARIABLES                                                           
SortedList_t * heads;
int opt_yield = 0;
int mutex = 0;
int spin_lock = 0;
pthread_mutex_t * locks = NULL;
int num_threads;
int num_iterations;
int num_lists = 1;
int * lock_spin = NULL;
int * thread_locks = NULL;
long long * total_threadLock_time = NULL;
int thread_placement = 0;
pthread_mutex_t time_lock;
void malloc_error();

void segfault_handler() {

  fprintf(stderr, "Error, segmentation fault occured, exiting...\n");
  exit(2);
}

void initialize_mutexes_and_heads()
{
  int count;
  locks = (pthread_mutex_t *) malloc(num_lists*sizeof(pthread_mutex_t));
  heads = (SortedList_t *) malloc(num_lists*sizeof(SortedList_t));
  lock_spin = (int *) malloc(num_lists*sizeof(int));
  thread_locks = (int *) malloc(num_threads*sizeof(int));
  total_threadLock_time = (long long *) malloc(num_threads*sizeof(long long));
  if (pthread_mutex_init(&time_lock, NULL) != 0)
    {
      fprintf(stderr, "Error initializing mutex, exiting...\n");
      exit(1);
    }
  for (count = 0; count < num_lists; count++)
    {
      if (pthread_mutex_init(&locks[count], NULL) != 0)
	{
	  fprintf(stderr, "Error initializing mutex, exiting...\n");
	  exit(1);
	}
      heads[count].next = &heads[count];
      heads[count].prev = &heads[count];
      heads[count].key = NULL;
    }
}

void malloc_error()
{
  fprintf(stderr,"Error mallocing memory: %s\n", strerror(errno));
  exit(1);
}

void destroy_mutex()
{
  int count;
  for (count = 0; count < num_lists; count++)
    {
      if (pthread_mutex_destroy(&locks[count]) != 0)
	{
	  fprintf(stderr, "Error destroying mutex, exiting...\n");
	  exit(1);
	}
    }
  if (pthread_mutex_destroy(&time_lock) != 0)
    {
      fprintf(stderr, "Error destroying mutex, exiting...\n");
      exit(1);
    }

}

void thread_error(char * message) {

  fprintf(stderr, "%s\n", message);
  exit(1);

}

void error_message(char * message) {
  fprintf(stderr, "%s: %s\n", message, strerror(errno));
  exit(1);
}

char * create_key()
{
  int curr;
  char * value = malloc(sizeof(char) * SIZE_OF_KEY);
  if (value == NULL)
    malloc_error();
  for (curr =0; curr < SIZE_OF_KEY-1; curr++)
    value[curr] = (rand() % 95) + 32;
  value[SIZE_OF_KEY-1] = '\0';

  return value;

}

void check_length(int len)
{    
  if (len == -1)
    {
      fprintf(stderr, "Elements not inputted into list properly: improper length, exiting...\n");
      exit(2);
    }
}

unsigned int choose_lock(SortedListElement_t * curr_element) {
  int i;
  unsigned int hash = 0;
  for (i =0; i < 10; i++)
    {
      hash += curr_element->key[i];  
    }
  
  unsigned int lock_num = (unsigned int) hash % num_lists;
  return lock_num;

}

void * run_thread(void * arg) {

  long long total_runtime = 0;
  int num_locks = 0;
  struct timespec start, finish;
  int to_nano = 1000000000;
  SortedListElement_t * current_element = arg;
  int i;
  int length;
  for(i =0; i < num_iterations; i++)
    {
      unsigned int lock_num = choose_lock(current_element + i);
      if (mutex == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
	    error_message("Error getting time");
	  pthread_mutex_lock(&locks[lock_num]);
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
	    error_message("Error getting time");
	  total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
	  num_locks+=1;
	  SortedList_insert(&heads[lock_num], current_element + i);
	  pthread_mutex_unlock(&locks[lock_num]);
	}
      else if (spin_lock == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
            error_message("Error getting time");
	  while(__sync_lock_test_and_set(&lock_spin[lock_num], 1));
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
            error_message("Error getting time");
          total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
          num_locks+=1;
          SortedList_insert(&heads[lock_num], current_element + i);
	  __sync_lock_release(&lock_spin[lock_num]);
	}
      else
	SortedList_insert(&heads[lock_num], current_element + i);
	
    }
  for (i =0; i < num_lists; i++)
    {
      if (mutex == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
            error_message("Error getting time");
	  pthread_mutex_lock(&locks[i]);
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
            error_message("Error getting time");
          total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
          num_locks+=1;
	  length = SortedList_length(&heads[i]);
	  check_length(length);
	  pthread_mutex_unlock(&locks[i]);
	}
      else if (spin_lock == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
            error_message("Error getting time");
	  while(__sync_lock_test_and_set(&lock_spin[i], 1));
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
            error_message("Error getting time");
          total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
	  num_locks+=1;
	  length = SortedList_length(&heads[i]);
	  check_length(length);
	  __sync_lock_release(&lock_spin[i]);
	}
      else
	{
	  length = SortedList_length(&heads[i]);
	  check_length(length);
	}
    }
  for (i =0; i < num_iterations; i++)
    {
      unsigned int lock_num = choose_lock(current_element + i);
      if (mutex == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
            error_message("Error getting time");
	  pthread_mutex_lock(&locks[lock_num]);
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
            error_message("Error getting time");
          total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
          num_locks+=1;
	  if (SortedList_delete(SortedList_lookup(&heads[lock_num],(current_element+i)->key)) == 1)
	    {
	      fprintf(stderr, "Error deleting element: element not found, exiting...\n");
	      exit(2);
	    }
	  pthread_mutex_unlock(&locks[lock_num]);
	}
      else if (spin_lock == 1)
	{
	  if (clock_gettime(CLOCK_REALTIME, &start) == -1)
            error_message("Error getting time");
	  while(__sync_lock_test_and_set(&lock_spin[lock_num], 1));
	  if (clock_gettime(CLOCK_REALTIME, &finish) == -1)
            error_message("Error getting time");
          total_runtime += (to_nano*(finish.tv_sec - start.tv_sec)) + (finish.tv_nsec - start.tv_nsec);
          num_locks+=1;
	  if (SortedList_delete(SortedList_lookup(&heads[lock_num],(current_element+i)->key)) == 1)
            {
              fprintf(stderr, "Error deleting element: element not found, exiting...\n");
              exit(2);
            }
	  __sync_lock_release(&lock_spin[lock_num]);
	}
      else
	{
	  if (SortedList_delete(SortedList_lookup(&heads[lock_num],(current_element+i)->key)) == 1)
            {
              fprintf(stderr, "Error deleting element: element not found, exiting...\n");
              exit(2);
            }
	}
    } 

  pthread_mutex_lock(&time_lock);
  total_threadLock_time[thread_placement] = total_runtime;
    thread_locks[thread_placement] = num_locks;
    thread_placement++;
  pthread_mutex_unlock(&time_lock);

  return NULL;
}

void print_results(int thread_nums, int it_nums, long long run_time) {
  int i;
  long long total_lock_time = 0;
  int total_locks;
  total_locks = ((num_iterations*2)+num_lists)*num_threads;
  //printf("%d\n", total_locks);
  for (i =0; i < num_threads; i++)
    {
      //printf("%lld\n", total_threadLock_time[i]);
      total_lock_time += total_threadLock_time[i];
    }

  long avg_wait_time = total_lock_time/total_locks;
  int number_ops = thread_nums*it_nums*3;
  int avg_time_per_op = run_time/number_ops;
  printf("list-");
  if (opt_yield & INSERT_YIELD)
    printf("i");
  if (opt_yield & DELETE_YIELD)
    printf("d");
  if (opt_yield & LOOKUP_YIELD)
    printf("l");
  if (opt_yield == 0)
    printf("none");

  if (mutex == 1)
    printf("-m,");
  else if (spin_lock == 1)
    printf("-s,");
  else
    printf("-none,");


  printf("%d,%d,%d,%d,%lld,%d,%ld\n",thread_nums, it_nums, num_lists, number_ops, run_time,avg_time_per_op, avg_wait_time);

}


int main(int argc, char * argv[]) {

  signal(SIGSEGV,segfault_handler);

  //Initialize mutex                                                                                                 
  //  initialize_mutex();

  int opt;
  //Number of threads and iterations, default 1                                                                      
  num_threads = 1;
  num_iterations = 1;

 struct option optional_args[] =
   {
     {"threads", required_argument, NULL, 't' },
     {"iterations", required_argument, NULL, 'i' },
     {"yield", required_argument, NULL, 'y' },
     {"sync", required_argument,NULL, 's'},
     {"lists", required_argument, NULL, 'l'},
     {0,0,0,0}
   };

 size_t check_yield;
 size_t length;
 while((opt = getopt_long(argc,argv,"",optional_args,NULL)) != -1)
   {
     switch(opt)
       {
       case 0:
         break;
       case 't':
         num_threads = atoi(optarg);
         break;
       case 'i':
         num_iterations = atoi(optarg);
         break;
       case 'y':
	 length = strlen(optarg);
	 for (check_yield =0; check_yield < length; check_yield++)
	   {
	     if (optarg[check_yield] == 'i')
	       opt_yield |= INSERT_YIELD;
	     else if (optarg[check_yield] == 'd')
	       opt_yield |= DELETE_YIELD;
	     else if (optarg[check_yield] == 'l')
	       opt_yield |= LOOKUP_YIELD;
	     else
	       {
		 fprintf(stderr, "Unkown input argument with '--yield', exiting...\n");
		 exit(1);
	       }
	   }
         break;
       case 'l':
	 num_lists = atoi(optarg);
	 if (num_lists < 1)
	   {
	     fprintf(stderr, "Error, number of lists must be greater than 0, exiting...\n");
	     exit(1);
	   }
	 break;
       case 's':
	 if (strcmp(optarg,"m") == 0)
	   mutex =1;
	 else if (strcmp(optarg,"s") == 0)
	   spin_lock = 1;
	 else
	   {
	     fprintf(stderr, "Error, unknown input for '--sync' option, exiting...");
	     exit(1);
	   }
	 break;
       case '?':
         fprintf(stderr, "Error, unknown input argument, '%s'.\n", argv[optind-1]);
         exit(1);
       }

   }

 initialize_mutexes_and_heads();

 SortedListElement_t * elements = malloc(num_threads*num_iterations*sizeof(SortedListElement_t));
 if (elements == NULL)
   malloc_error();
 char ** list_of_keys = malloc(num_iterations*num_threads*sizeof(char*));
 if (list_of_keys == NULL)
   malloc_error();
 /* Generates random keys for each of the elements */
 int j;
 for (j =0; j < (num_threads*num_iterations); j++) {
   elements[j].key = create_key();
   list_of_keys[j] = (char *) elements[j].key;
 }
 pthread_t* threads = malloc(num_threads*sizeof(pthread_t));
 int i;
 int check;

 //Start time                                                                                                                                   
 struct timespec start_time, end_time;
 if (clock_gettime(CLOCK_REALTIME, &start_time) == -1)
   error_message("Error getting time");

 for (i = 0; i < num_threads; i++)
   {
     check = pthread_create(&threads[i], NULL, &run_thread, (void *) (elements+(i*num_iterations)));
     if (check != 0)
       thread_error("Error creating thread, exiting...");
   }

 for (i =0; i < num_threads; i++)
   {
     check = pthread_join(threads[i],NULL);
     if (check != 0)
       thread_error("Error waiting for thread to end, exiting...");
   }

 for (i=0; i < num_lists; i++)
   {
     if (SortedList_length(&heads[i]) != 0)
       {
	 fprintf(stderr, "Error, elements not deleted from list properly, exiting...");
	 exit(2);
       }
   }
 //End Time                                                                                                                                     
 if (clock_gettime(CLOCK_REALTIME, &end_time) == -1)
   error_message("Error getting time");


 int convert_to_nano = 1000000000;
 long long total_runtime = (convert_to_nano*(end_time.tv_sec - start_time.tv_sec)) + (end_time.tv_nsec - start_time.tv_nsec);

   print_results(num_threads,num_iterations, total_runtime);

   destroy_mutex();
   free(threads);
   for (i =0; i < (num_threads*num_iterations); i++)
     free(list_of_keys[i]);
   free (elements);
   free(heads);      
   free(locks);
 

 exit(0);
}
