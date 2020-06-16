
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <time.h>

//GLOBAL VARIABLES
long long counter = 0;
int opt_yield = 0;
int mutex = 0;
int spin_lock = 0;
int atomic = 0;
pthread_mutex_t lock;


void add(long long *pointer, long long value) {
  if (mutex == 1)
    {
      pthread_mutex_lock(&lock);
      long long sum = *pointer + value;
      if (opt_yield)
	{
	  sched_yield();
	}
      *pointer = sum;
      pthread_mutex_unlock(&lock);
    }
  else if (spin_lock == 1)
    {
      static int lock_spin;
      while(__sync_lock_test_and_set(&lock_spin, 1));
      long long sum = *pointer + value;
      if (opt_yield)
        {
          sched_yield();
        }
      *pointer = sum;
      __sync_lock_release(&lock_spin);
    }
  else if (atomic == 1)
    {
      long long temp;
      do {
      temp = *pointer;
      if (opt_yield)
	sched_yield();
      }while(__sync_val_compare_and_swap(pointer, temp, temp+value) != temp);
    }
}

void * threadHelper(void * arg) {

  int iterate = *((int *) arg);
  int i;
  for (i = 0; i < iterate; i++)
    add(&counter, 1);
  for (i = 0; i < iterate; i++)
    add(&counter, -1);
  return NULL;

}

void initialize_mutex()
{
  if (pthread_mutex_init(&lock, NULL) != 0)
    {
      fprintf(stderr, "Error initializing mutex, exiting...\n");
      exit(1);
    }
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

void print_results(char * test_name, int thread_nums, int it_nums, long long run_time) {

  int number_ops = thread_nums*it_nums*2;
  int avg_time_per_op = run_time/number_ops;

  printf("%s,%d,%d,%d,%lld,%d,%lld\n", test_name, thread_nums, it_nums, number_ops, run_time,avg_time_per_op, counter);

}

int main(int argc, char * argv[]) {

  //Initialize mutex
  initialize_mutex();

  int opt;
  //Number of threads and iterations, default 1
  int num_threads = 1;
  int num_iterations = 1;

 struct option optional_args[] =
   {
     {"threads", required_argument, NULL, 't' },
     {"iterations", required_argument, NULL, 'i' },
     {"sync", required_argument, NULL, 's' },
     {"yield", no_argument, &opt_yield, 1 },
     {0,0,0,0}
   };


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
       case 's':
	 if (strcmp(optarg,"m") == 0)
	   mutex = 1;
	 else if (strcmp(optarg,"s") == 0)
	   spin_lock = 1;
	 else if (strcmp(optarg,"c") == 0)
	   atomic = 1;
	 else
	   {
	     fprintf(stderr, "Unkown input argument with '--sync', exiting...\n");
	     exit(1);
	   }
	 break;
       case '?':
	 fprintf(stderr, "Error, unknown input argument, '%s'.\n", argv[optind-1]);
	 exit(1);

       }

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
     check = pthread_create(&threads[i], NULL, &threadHelper, (void *) &num_iterations);
     if (check != 0)
       thread_error("Error creating thread, exiting...");
   }

 for (i =0; i < num_threads; i++)
   {
     check = pthread_join(threads[i],NULL);
     if (check != 0)
       thread_error("Error waiting for thread to end, exiting...");
   }

 //End Time
 if (clock_gettime(CLOCK_REALTIME, &end_time) == -1)
   error_message("Error getting time");

 int convert_to_nano = 1000000000;
 long long total_runtime = (convert_to_nano*(end_time.tv_sec - start_time.tv_sec)) + (end_time.tv_nsec - start_time.tv_nsec);

 if (opt_yield && spin_lock)
   print_results("add-yield-s",num_threads,num_iterations, total_runtime);
 else if (opt_yield && mutex)
   print_results("add-yield-m",num_threads,num_iterations, total_runtime);
 else if (opt_yield && atomic)
   print_results("add-yield-c",num_threads,num_iterations, total_runtime);
 else if (opt_yield)
   print_results("add-yield-none",num_threads,num_iterations, total_runtime);
 else if (mutex)
  print_results("add-m",num_threads,num_iterations, total_runtime);
 else if (spin_lock)
   print_results("add-s",num_threads,num_iterations, total_runtime);
 else if (atomic)
   print_results("add-c",num_threads,num_iterations, total_runtime);
 else
   print_results("add-none",num_threads,num_iterations, total_runtime);


 destroy_mutex();
  free(threads);
  
  exit(0);

}
