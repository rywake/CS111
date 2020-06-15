/*
NAME:Ryan Wakefield
EMAIL: ryanwakefield@g.ucla.edu
ID: 904975422
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#define NUM_BYTES 100


/* Handles error message for catching segfault */
void sighandler(int signum)
{
  signum = 4;
  fprintf(stderr,"Caught segmentation fault from signal, '--catch' used as input argument, exiting with exit code %d.\n", signum);
  exit(4);
}

/* Open and redirects input file or throws an error if can't be opened. */
void redirect_input(char * file)
{
  int ifd = open(file, O_RDONLY);
  if (ifd >= 0)
    {
      close(0);
      dup(ifd);
      close(ifd);
    }
  else
    {
      fprintf(stderr, "Error, unable to open input file '%s': %s\n", file, strerror(2));
      exit(2);
    }
}

/* Opens or creates and redirects output file or throws an error if can't be opened. */
void redirect_output(char * file)
{
  int ofd = creat(file, 0666);
  if (ofd >= 0)
    {
      close(1);
      dup(ofd);
      close(ofd);
    }
  else
    {
      fprintf(stderr, "Error, unable to create/open output file '%s': %s\n", file, strerror(3));
      exit(3);
    }
}

int main(int argc, char* argv[]) {

  int opt; //Holds the value returned from getopt_long function

  /* Stores information on what arguments were inputted */
  struct has_arg {
    char * inp_file;
    char * out_file;
    int has_segfault;
    int has_catch;

  } input_args;


  /*Initializes has_segfault and has_catch to 0, representing false, and input and output files to NULL. */
  input_args.has_segfault = 0;
  input_args.has_catch = 0;
  input_args.inp_file = NULL;
  input_args.out_file = NULL;


  struct option optional_args[] =
    {
      {"input", required_argument, NULL, 'i' },
      {"output", required_argument, NULL, 'o' },
      {"segfault", no_argument, &input_args.has_segfault, 1 },
      {"catch", no_argument, &input_args.has_catch, 1},
      {0,0,0,0}
    };



  /*Parses the arguments inputted through the command line. */
  while ((opt = getopt_long(argc,argv, "", optional_args, NULL )) != -1)
    {
      switch(opt)
	{
	case 0:
	  break;
	case 'i':
	  input_args.inp_file = optarg;
	  break;
	case 'o':
	  input_args.out_file = optarg;
	  break;
	case '?':
	  fprintf(stderr, "Error, unknown input argument, '%s'.\n", argv[optind-1]);
	   exit(1);
	}

    }

  /* Redirects the input and output files, if any were inputted. */
  if (input_args.inp_file != NULL)
    redirect_input(input_args.inp_file);
  if(input_args.out_file != NULL)
    redirect_output(input_args.out_file);


  /* Performs necessary functions if '--catch' was inputted as an argument. */
  if (input_args.has_catch)
    {
      signal(SIGSEGV,sighandler);
    }


  /* Performs necessary function if '--segfault' was inputted as an argument. */
  if (input_args.has_segfault)
    {
      char * seg_fault = NULL;
      int fail = * seg_fault;
      return fail;
    }


  char transfer_from_input[NUM_BYTES];
 
  int check_read = read(0, transfer_from_input, NUM_BYTES);

  while (check_read > 0)
  {
      write(1, transfer_from_input, check_read);
      check_read = read(0, transfer_from_input, NUM_BYTES);
  }

  exit(0);

}
