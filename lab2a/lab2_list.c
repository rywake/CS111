
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
SortedList_t head;
int opt_yield = 0;
int mutex = 0;
int spin_lock = 0;
pthread_mutex_t lock;
int num_threads;
int num_iterations;
void malloc_error();

void segfault_handler() {

  fprintf(stderr, "Error, segmentation fault occured, exiting...\n");
  exit(2);
}

void initialize_mutex()
{
  if (pthread_mutex_init(&lock, NULL) != 0)
    {
      fprintf(stderr, "Error initializing mutex, exiting...\n");
      exit(1);
    }
}

void malloc_error()
{
  fprintf(stderr,"Error mallocing memory: %s\n", strerror(errno));
  exit(1);
}

void destroy_mutex()
{
  if (pthread_mutex_destroy(&lock) != 0)
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

void * run_thread(void * arg) {

  SortedListElement_t * current_element = arg;
  int i;
  static int lock_spin;
  int length;
  for(i =0; i < num_iterations; i++)
    {
      //      printf("%s\n",(current_element+i)->key);
      if (mutex == 1)
	{
	  pthread_mutex_lock(&lock);
	  SortedList_insert(&head, current_element + i);
	  pthread_mutex_unlock(&lock);
	}
      else if (spin_lock == 1)
	{
	  while(__sync_lock_test_and_set(&lock_spin, 1));
          SortedList_insert(&head, current_element + i);
	  __sync_lock_release(&lock_spin);
	}
      else
	SortedList_insert(&head, current_element + i);
	
    }

  if (mutex == 1)
    {
      pthread_mutex_lock(&lock);
      length = SortedList_length(&head);
      pthread_mutex_unlock(&lock);
    }
  else if (spin_lock == 1)
    {
      while(__sync_lock_test_and_set(&lock_spin, 1));
      length = SortedList_length(&head);
      __sync_lock_release(&lock_spin);
    }
  else
    length = SortedList_length(&head);

  if (length == -1)
    {  
      fprintf(stderr, "Elements not inputted into list properly, exiting...\n");         
      exit(2);
    }   
 
  for (i =0; i < num_iterations; i++)
    {
      if (mutex == 1)
	{
	  pthread_mutex_lock(&lock);
	  if (SortedList_delete(SortedList_lookup(&head,(current_element+i)->key)) == 1)
	    {
	      fprintf(stderr, "Error deleting element: element not found, exiting...\n");
	      exit(2);
	    }
	  pthread_mutex_unlock(&lock);
	}
      else if (spin_lock == 1)
	{
	  while(__sync_lock_test_and_set(&lock_spin, 1));
	  if (SortedList_delete(SortedList_lookup(&head,(current_element+i)->key)) == 1)
            {
              fprintf(stderr, "Error deleting element: element not found, exiting...\n");
              exit(2);
            }
	  __sync_lock_release(&lock_spin);
	}
      else
	{
	  if (SortedList_delete(SortedList_lookup(&head,(current_element+i)->key)) == 1)
            {
              fprintf(stderr, "Error deleting element: element not found, exiting...\n");
              exit(2);
            }
	}
    } 

  return NULL;
}

void print_results(int thread_nums, int it_nums, long long run_time) {

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

  int lists = 1;

  printf("%d,%d,%d,%d,%lld,%d\n",thread_nums, it_nums, lists, number_ops, run_time,avg_time_per_op);

}


int main(int argc, char * argv[]) {

  signal(SIGSEGV,segfault_handler);

  initialize_mutex();

  head.next = &head;
  head.prev = &head;
  head.key = NULL;

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

 if (SortedList_length(&head) != 0)
   {
     fprintf(stderr, "Error, elements not deleted from list properly, exiting...");
     exit(2);
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

 exit(0);
}
