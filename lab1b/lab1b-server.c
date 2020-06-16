
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <ulimit.h>
#include <zlib.h>
#include <fcntl.h>

#define NBYTES 1024
#define CAR_C 0x03

z_stream out_stream; //compress from keyboard to shell                                                                    
z_stream in_stream; //compress from shell to display           
char buffer[NBYTES]; //buffer to store the data read in                                                                                         


void decompress_data(int count);
void compress_data(int count);
void check_error_poll(int check_error);
void check_error_read(int check);
void check_error_write(int check);

/* handles the ^C case when it is sent to shell */
void sighandler(int signum) {

  fprintf(stderr,"Signal SIGPIPE received: %d\n", signum);

}

/*Writes back to the socket */
void write_to_stdout(int fds,int compress_me) {
  int check; //Check for errors/how many bytes were processed 
  check = read(fds,&buffer, NBYTES);
  if (compress_me == 1)
    {
      compress_data(check);
      write(0,&buffer, NBYTES - out_stream.avail_out);
    }
  else
    write(0,&buffer,check);


}

/*Writes back to the server */
void write_to_shell(int fds, int fds2, struct pollfd poll_fds[2], pid_t pid_child, int compress) {

  //  char buffer[NBYTES]; //buffer to store the data read in  
  int check = 1; //Check for errors/how many bytes were processed 
  int check_poll; //Checks for errors in the poll 
  int i; //iterator to loop through characters stored in buffer 
  char c; //checks each character for special characters to handle.  
  int check_write; //checks for errors in writing   
  while(check > 0)
    {
      check_poll = poll(poll_fds,2,0);
      check_error_poll(check_poll);
      if (check_poll > 0)
        {
          if(poll_fds[0].revents & (POLLIN)) //Read from the socket
            {
              check = read(0, &buffer, NBYTES);
	      if (compress == 1)
		{
		  decompress_data(check);
		  check = NBYTES - in_stream.avail_out;
		}
              for (i =0; i < check; i++)
                {
                  c = buffer[i];
                  check_error_read(check);
                  if (c == '\004')
                    {
                      close(fds);
                      while ((check = read(fds2, &buffer, NBYTES)) > 0)
                        write(0,&buffer, check); //write back to the socket
                      close(fds2);
                      return;
		    }
                  else if (c == CAR_C)
                    {
                      kill(pid_child, SIGINT);
                    }
                  else if (c == '\r' || c == '\n')
                    {
                     
                      char newline_shell = '\n';
                      check_write = write(fds, &newline_shell,1); //Writes to the shell
                      check_error_write(check_write);

                    }
                  else
                    {
                      check_write = write(fds, &c, 1);
                      check_error_write(check_write);
                    }
                }
            }
	  else if (poll_fds[1].revents & (POLLIN)) //Writes to the socket
            {
              write_to_stdout(fds2, compress);
            }
          else if (poll_fds[0].revents & (POLLHUP|POLLERR)) //Writes to the socket
            {
              close(fds);
              while ((check = read(fds2, &buffer, NBYTES)) > 0)
		{
		  if (compress == 1)
		    {
		      compress_data(check);
		      write(0,&buffer, NBYTES - out_stream.avail_out);
		    }
		  else
		    write(0,&buffer, check);
		}
              close(fds2);
              return;
            }
          else if (poll_fds[1].revents & (POLLHUP|POLLERR)) //Writes to the socket
            {
              close(fds);
              while ((check = read(fds2, &buffer, NBYTES)) > 0)
		{
		  if (compress == 1)
                    {
                      compress_data(check);
                      write(0,&buffer, NBYTES - out_stream.avail_out);
                    }
		  else
		    write(0,&buffer, check);
		}
              close(fds2);
              return;
            }

        } //if(check_poll)                                                                        
    } //while                                                                                                             
}

void check_error_poll(int check_error)
{
  if (check_error < 0)
    {
      fprintf(stderr, "Error running poll: %s\n", strerror(errno));
     
      exit(1);
    }

}
void check_error_read(int check)
{
  if (check < 0)
    {
      fprintf(stderr, "Error with read system call: %s\n", strerror(errno));
     
      exit(1);
    }

}

void check_error_write(int check)
{
  if (check < 0)
    {
      fprintf(stderr, "Error while writing: %s\n", strerror(errno));
     
      exit(1);
    }

}

/* Compresses the data before sending it to the socket */
void compress_data(int count) {

  char temp[NBYTES];
  memcpy(temp,buffer,count);

  out_stream.avail_in = count;
  out_stream.next_in = (Bytef *) temp;
  out_stream.avail_out = NBYTES;
  out_stream.next_out = (Bytef *) buffer;
  
    do {
      deflate(&out_stream, Z_SYNC_FLUSH);
    }   while (out_stream.avail_in > 0);

}

/*Decompresses the data after reading it from the socket */
void decompress_data(int count)
{

  char temp[NBYTES];
  memcpy(temp,buffer,NBYTES);

  in_stream.avail_in = count;
  in_stream.next_in = (Bytef *) temp;
  in_stream.avail_out = NBYTES;
  in_stream.next_out = (Bytef *) buffer;


 
  do{
     inflate(&in_stream, Z_SYNC_FLUSH);
  } while (in_stream.avail_in > 0);
  


}

/* Creates the socket to communicate with the shell */
int create_socket() {

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, " Error creating the socket, exiting...\n");
      exit(1);
    }

  return sockfd;
}


int main(int argc, char * argv[]) {

  signal(SIGPIPE, sighandler); //Handles ^C case

  /* Sets up z_stream values needed to compress and decompress data */
  out_stream.zalloc = Z_NULL;
  out_stream.zfree = Z_NULL;
  out_stream.opaque = Z_NULL;
  out_stream.avail_in = 0;
  out_stream.next_in = Z_NULL;

  in_stream.zalloc = Z_NULL;
  in_stream.zfree = Z_NULL;
  in_stream.opaque = Z_NULL;

  int deflate_return = deflateInit(&out_stream, Z_DEFAULT_COMPRESSION);
  if (deflate_return != Z_OK)
    {
      fprintf(stderr, "Unable to create compression stream: %s\n", strerror(errno));
      exit(1);
    }

  int inflate_return = inflateInit(&in_stream);
  if (inflate_return != Z_OK)
    {
      fprintf(stderr, "Unable to create decompression stream: %s\n", strerror(errno));
      exit(1);
    }

  int opt;
  int port;
  int has_compress = 0;
  int gave_port = 0;
  int pipe1[2]; //terminal to shell                                                                                               
  int pipe2[2]; //shell to terminal    
  //  int has_shell_arg = 0; //checks to see if shell arg was inputted.                                                                
  char * value = NULL;
  struct sockaddr_in address;
  int sockfd, new_socket, address_length = sizeof(address);
  memset(&address, '0', sizeof(address));


  /* This part of code handles the getopt parsing of optional argument */
  /* ================================================================= */

  struct option client_args[] =
    {
      {"port", required_argument, NULL, 'p' },
      {"compress", no_argument, &has_compress, 1},
      {"shell",required_argument,NULL, 's' }, //Do i need to add the shell in the server?
      {0,0,0,0}
    };


  /*Parses the arguments inputted through the command line. */
  while ((opt = getopt_long(argc,argv, "", client_args, NULL )) != -1)
    {
      switch(opt)
	{
	case 0:
	  break;
	case 'p':
	  gave_port = 1;
	  port = atoi(optarg);
	  break;
	case 's':
	  //	  has_shell_arg = 1;
	  value = optarg;
	  break;
	case '?':
	  fprintf(stderr, "Error, unknown input argument, '%s'.\n", argv[optind-1]);
	  exit(1);
	}
      
    }


  if (gave_port == 0)
    {
      fprintf(stderr, "Error, no port was given, exiting...\n");
      exit(1);
    }
  
/* End of part of code that handles the parsing of argument */
/* ======================================================== */
  sockfd = create_socket();

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; //Maps to IP address 127.0.0.1
  address.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr * ) &address, address_length) < 0)
    {
      fprintf(stderr, "Error while binding: %s\n", strerror(errno));
      exit(1);
    }

  if (listen(sockfd, 5) < 0)
    {
      fprintf(stderr, "Error while listening, exiting...\n");
      exit(1);
    }

  new_socket = accept(sockfd, (struct sockaddr *) &address, (socklen_t *) &address_length);
  if (new_socket < 0)
    {
      fprintf(stderr, "Error while accepting, exiting...\n");
      exit(1);
    }

  /* Starting code for the shell process */
  /*==================================== */

  /* Creates a pipe to communicate bn child and parent processes. */
  if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
      fprintf(stderr, "Error creating the pipes, exiting...");
     
      exit(1);
    }

  /* Forks the process */

      pid_t pid = fork(); //Creates a new process for the executable                                                                  
      if (pid < 0)
        {
          fprintf(stderr, "Failed forking, exiting...\n");
        
          exit(1);
        }
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

	  close(0);
	  dup(new_socket);
	  //	  close(new_socket);
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
          write_to_shell(pipe1[1], pipe2[0],fds, pid, has_compress);

          int child_status; //Stores information about the exit status of the child
          waitpid(pid,&child_status, 0); //Waits for the shild process to finish executing
          int lower_bits = (child_status & 0x007f); //Correspnds to signal value 
          int second_byte = ((child_status & 0xff00) >> 8); //Corresponds to exit status value;    
	   

	  /*Shuts down the socket before exiting */
	  if ((shutdown(new_socket, SHUT_RDWR)) < 0)
	    {
	      fprintf(stderr, "Error shutting down socket: %s\n", strerror(errno));
	      exit(1);
	    }
	  
	  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", lower_bits, second_byte);
        }

      deflateEnd(&out_stream);
      inflateEnd(&in_stream);
     
      exit(0);


}
