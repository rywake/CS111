/*
NAME: Ryan Wakefield
EMAIL: ryanwakefield@g.ucla.edu
ID: 904975422
*/


#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define NBYTES 256
#define CAR_C 0x03 //MACRO to define ^C

void check_error_poll(int check_error, struct termios original_Attributes);
void check_error_read(int check, struct termios original_Attributes);
void check_error_write(int check, struct termios original_Attributes);

void sighandler(int signum) {

  fprintf(stderr,"Signal SIGPIPE received: %d\n", signum); 

}

void write_to_stdout(int fds, struct termios original_Attributes) {
  char buffer[NBYTES]; //buffer to store the data read in     
  int check; //Check for errors/how many bytes were processed    
  int i; //iterator to loop through characters stored in buffer 
  int check_write; //Check for errors while writing
  check = read(fds,&buffer, NBYTES);

  for (i =0; i < check; i++) //loops through characters read in
    {
      char c = buffer[i];
      if (c == '\n')
	{
	  char newline[2] = {'\r', '\n'};
	  check_write = write(1, newline, 2);
	  check_error_write(check_write, original_Attributes);
	}
      else
	{
	  check_write = write(1,&c, 1);
	  check_error_write(check_write, original_Attributes);
	}
    }
}

void write_to_shell(int fds, int fds2, struct termios original_Attributes, struct pollfd poll_fds[2], pid_t pid_child) {

  char buffer[NBYTES]; //buffer to store the data read in
  int check = 1; //Check for errors/how many bytes were processed
  int check_poll; //Checks for errors in the poll
  int i; //iterator to loop through characters stored in buffer
  char c; //checks each character for special characters to handle.
  int check_write; //checks for errors in writing
  
  while(check > 0)
    {
      check_poll = poll(poll_fds,2,0);
      check_error_poll(check_poll, original_Attributes);
      if (check_poll > 0)
	{
	  if(poll_fds[0].revents & (POLLIN))
	    { 

	      check = read(0, &buffer, NBYTES);

	      for (i =0; i < check; i++)
		{
		  c = buffer[i];
		  check_error_read(check,original_Attributes );
		  if (c == '\004')
		    {
		      write(1,"^D", 2);
		      close(fds);
		      while ((check = read(fds2, &buffer, NBYTES)) > 0)
			write(1,&buffer, check);
		      close(fds2);
		      return;

		    }
		  else if (c == CAR_C)
		    {
		      write(1, "^C", 2);
		      kill(pid_child, SIGINT);
		    }
		  else if (c == '\r' || c == '\n')
		    {
		      char newline[2] = {'\r', '\n'};
		      char newline_shell = '\n';
		      check_write = write(1, newline, 2);
		      check_error_write(check_write, original_Attributes);
		      check_write = write(fds, &newline_shell,1);
		      check_error_write(check_write, original_Attributes);
		      
		    }
		  else
		    {
		      check_write = write(fds, &c, 1);
		      check_error_write(check_write, original_Attributes);
		      check_write = write(1, &c, 1);
                      check_error_write(check_write, original_Attributes);
		    }
		}
	    }
	  else if (poll_fds[1].revents & (POLLIN))
	    {
	      write_to_stdout(fds2, original_Attributes);
	    }
	  else if (poll_fds[0].revents & (POLLHUP|POLLERR))
	    {
	      close(fds);
	      while ((check = read(fds2, &buffer, NBYTES)) > 0)
		write(1,&buffer, check);
	      close(fds2);
	      return;
	    }
	  else if (poll_fds[1].revents & (POLLHUP|POLLERR))
	    {
	      close(fds);
	      while ((check = read(fds2, &buffer, NBYTES)) > 0)
		write(1,&buffer, check);
	      close(fds2);
              return;
	    }

	} //if(check_poll)

    } //while
}

void check_error_poll(int check_error, struct termios original_Attributes)
{
  if (check_error < 0)
    {
      fprintf(stderr, "Error running poll: %s\n", strerror(errno));
      tcsetattr(0,TCSANOW,&original_Attributes);
      exit(1);
    }

}
void check_error_read(int check, struct termios original_Attributes)
{
  if (check < 0)
    {
      fprintf(stderr, "Error with read system call: %s\n", strerror(errno));
      tcsetattr(0,TCSANOW,&original_Attributes);
      exit(1);
    }

}

void check_error_write(int check, struct termios original_Attributes)
{
  if (check < 0)
    {
      fprintf(stderr, "Error while writing: %s\n", strerror(errno));
      tcsetattr(0,TCSANOW,&original_Attributes);
      exit(1);
    }

}



void set_attributes() {

  /* Creates a struct with all of the attributes of terminal properties */ 
struct termios t_Attributes;
tcgetattr(0,&t_Attributes);


/* Terminal parameters to be set */
t_Attributes.c_iflag = (ISTRIP);
t_Attributes.c_oflag = 0;
t_Attributes.c_lflag = 0;

/* Set the new parameters to the terminal */
tcsetattr(0,TCSANOW,&t_Attributes);

}


 int main(int argc, char* argv[])
{

  signal(SIGPIPE, sighandler);

  /* Saves the original terminal properties */
  struct termios original_attributes;
  tcgetattr(0,&original_attributes);
  /*----------------------------*/

  set_attributes();

  /* Defines variables to be used for the pipe and getopt functions */

  int pipe1[2]; //terminal to shell
  int pipe2[2]; //shell to terminal
  int opt; //checks to see if the inputted argument in the command line is valid.
  int has_shell_arg = 0; //checks to see if shell arg was inputted.
  char * value = NULL; 
  int check; //Check to see how many characters were read in
  char buffer[NBYTES]; //Buffer to store data being transferred.



  /* Get opt struct to check for shell argument */

  struct option optional_arg[] =
    {
      {"shell",required_argument,&has_shell_arg, 1 },
      {0,0,0,0}
    };

  /* Checks for the --shell argument. */
  while ((opt = getopt_long(argc,argv, "", optional_arg, NULL)) != -1)
   {
     switch(opt)
       {
       case 0:
	 value = optarg;
	 break;
       case '?':
	 fprintf(stderr, "Error, unknown input argument, '%s'.\n",argv[optind-1]);
	 exit(1);
       }
   }

  /*-------------------------------*/



  /* Creates a pipe to communicate bn child and parent processes. */
  if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
      fprintf(stderr, "Error creating the pipes, exiting...");
      exit(1);
    }

  /* Forks the process if the --shell option is present */
  if (has_shell_arg == 1)
    {

      pid_t pid = fork(); //Creates a new process for the executable
     
      if (pid == 0) //This is the child process
	{
       
	  /* Changing the file directions for the child process */
	  close(0);
	  dup(pipe1[0]);
	  close(1);
      	  dup(pipe2[1]);
	  close(2);
	  dup(pipe2[1]);

	  close(pipe1[1]);
	  close(pipe2[0]);
	  close(pipe1[0]);
	  close(pipe2[1]);
	  /* =========================================== */
	  
	  /* Executes whatever function was inputted.  */
	  execlp(value, value, (char *) NULL );
   
	}

      if (pid > 0) //This is the parent process
	{

	  /* Creates and initializes data structures for poll */
	  struct pollfd fds[2];

	  fds[0].fd = 0;
	  fds[0].events = POLLIN|POLLHUP|POLLERR;

	  fds[1].fd = pipe2[0];
	  fds[1].events = POLLIN|POLLHUP|POLLERR;
	  
	  
	  /*Closes pipes not used by this process */
	  close(pipe1[0]);
	  close(pipe2[1]);

	  /*Writes to and from the the shell */
	  write_to_shell(pipe1[1], pipe2[0], original_attributes,fds, pid);
	  
	  int child_status; //Stores information about the exit status of the child

	  waitpid(pid,&child_status, 0); //Waits for the shild process to finish executing
	  
	  int lower_bits = (child_status & 0x007f); //Correspnds to signal value	  	  
	  int second_byte = ((child_status & 0xff00) >> 8); //Corresponds to exit status value;
	  
       fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", lower_bits, second_byte);
	}

    }
  else
    {
      int count;
      int continue_processing = 1;
      while(continue_processing)
	{
	  check = read(0, &buffer, NBYTES); 
	  for(count = 0; count < check; count++)
	    {
	      char curr_buff = buffer[count];
	      if (check < 0) {
		fprintf(stderr, "Error reading, exiting...");
		tcsetattr(0,TCSANOW,&original_attributes);
		exit(1);
	      }
	      if (curr_buff == '\004') {
		write(1,"^D", 2);
		continue_processing = 0;
		break;
	      }
	      if (curr_buff == '\r' || curr_buff == '\n')
		{
		  char newline[2] = {'\r', '\n'};
		  write(1, newline, 2);
		}
	      else
		write(1,&curr_buff,1);
	    }
	}
    }
  
  tcsetattr(0,TCSANOW,&original_attributes);
  exit(0);

}


