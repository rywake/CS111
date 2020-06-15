/*
NAME: Ryan Wakefield
EMAIL: ryanwakefield@g.ucla.edu
ID: 904975422
*/


#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <zlib.h>
#include <ulimit.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NBYTES 1024
#define CAR_C 0x03

char buffer[NBYTES];
z_stream out_stream; // client to server
z_stream in_stream; //server to client
int log_file; //Used if there is a log option
int is_log = 0; //if the log option is inputted
/* Saves the original terminal properties */
struct termios global_attributes;
/*----------------------------*/


void write_to_log(int sending, int value);

/* Creates the socket that will be used to communicate with the server */
int create_socket(struct termios original_Attributes) {

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, " Error creating the socket, exiting...\n");
      tcsetattr(0,TCSANOW,&original_Attributes);
      exit(1);
    }

  return sockfd;
}

/* Sets the attributes to non-canonical mode */
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

/* Compresses the data before sending it to the socket */
void compress_data(int count) {

  char temp[NBYTES];
  memcpy(temp,buffer,count);

  out_stream.avail_in = count;
  out_stream.next_in = (Bytef *) temp;
  out_stream.avail_out = NBYTES;
  out_stream.next_out = (Bytef *) buffer;


    do{
      deflate(&out_stream,Z_SYNC_FLUSH);
    }  while (out_stream.avail_in > 0);

}

/* Decompresses the data that came from the socket */
void decompress_data(int count)
{
  char temp[NBYTES];
  memcpy(temp,buffer,count);

  in_stream.avail_in = count;
  in_stream.next_in = (Bytef *) temp;
  in_stream.avail_out = NBYTES;
  in_stream.next_out = (Bytef *) buffer;


    do {
    inflate(&in_stream, Z_SYNC_FLUSH);
    }   while(in_stream.avail_in > 0);


}

/* Writes to the socket and then read in from the socket to the terminal*/
void write_to_server(int socket, struct termios original_Attributes, struct pollfd poll_fds[2], int compress)
{
  int check = 1;
  //  char buffer[NBYTES];
  int check_poll;
  int check_write;
  int i = 0;
  while (check > 0)
    {
      check_poll = poll(poll_fds, 2, 0);
      if (check_poll < 0)
	{
	  fprintf(stderr, "Error running poll: %s\n", strerror(errno));
	  tcsetattr(0,TCSANOW,&original_Attributes);
	  exit(1);
	}
      if  (check_poll > 0)
	{
	  if (poll_fds[0].revents & POLLIN) //Input from terminal
	    {
	      check = read(0,&buffer, NBYTES);
	      if (check < 0)
		{
		  fprintf(stderr, "Error reading from standard input: %s\n", strerror(errno));
		  tcsetattr(0,TCSANOW,&original_Attributes);
		  exit(1);
		}
	      
	      for (i =0; i < check; i ++)
		{
		  char c = buffer[i];
		  if ( c == '\r' || c == '\n')
		    {
		      char newline[2] = {'\r','\n' };
		      write(1, newline, 2);
		      if (check_write < 0)
			{
			  fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
			  tcsetattr(0,TCSANOW,&original_Attributes);
			  exit(1);
			}
		    }
		  else
		    {
		      check_write = write(1, &c, 1);
		      if (check_write < 0)
			{
			  fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
			  tcsetattr(0,TCSANOW,&original_Attributes);
			  exit(1);
			}
		    }
		}
	      if (compress == 1) //Compress
                {
                  compress_data(check);
                  check_write = write(socket, &buffer, NBYTES - out_stream.avail_out);
		  if (is_log == 1)
		    {
		      write_to_log(1,NBYTES-out_stream.avail_out);
		    }
                }
              else
		{
		  check_write = write(socket, &buffer, check);
		  if (is_log == 1)
		    {
		      write_to_log(1,check);
		    }
		}
              if (check_write < 0)
                {
                  fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
                  tcsetattr(0,TCSANOW,&original_Attributes);
                  exit(1);
                }
	    }
	  else if (poll_fds[1].revents & POLLIN) //Ready to read from the socket
	    {
	      check = read(socket, &buffer, NBYTES);
	      if (check < 0)
		{
		  fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
		  tcsetattr(0,TCSANOW,&original_Attributes);
		  exit(1);
		}
	      if (is_log == 1)
                {
                  write_to_log(0,check);
                }
	      if (compress == 1)
		{
		  decompress_data(check);
		  check = NBYTES - in_stream.avail_out;
		}
	      for (i = 0; i < check; i++)
		{	      
		  char c = buffer[i];                                                                                                  
		  if (c == '\n')                                                                                                       
		    {                                                                                                                  
		      char newline[2] = {'\r', '\n'};                                                                                  
		      check_write = write(1, newline, 2);                                                                              
		      if (check_write < 0)
			{
			  fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
			  tcsetattr(0,TCSANOW,&original_Attributes);
			  exit(1);
			}
		    }                                                                                                                  
		  else                                                                                                                 
		    {                                                                                                                  
		      check_write = write(1,&c, 1);                                                                                    
		      if (check_write < 0)
			{
			  fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
			  tcsetattr(0,TCSANOW,&original_Attributes);
			  exit(1);
			}
		    }          

		}
	    }
	  else if (poll_fds[1].revents & (POLLHUP|POLLERR)) //socket is closed, finishes reading in anything from the socket
	    {
	      while ((check = read(socket,buffer, NBYTES)) > 0)
		{
		  if (check < 0)
		    {
		      fprintf(stderr, "Error while reading from socket, exiting...");
                      tcsetattr(0,TCSANOW,&original_Attributes);
		      exit(1);
		    }
		  if (is_log == 1)
		    {
		      write_to_log(0,check);
		    }
		  if (compress == 1)
		    {
		      decompress_data(check);
		      check = NBYTES - in_stream.avail_out;
		    }
	      
	      for (i = 0; i < check; i++)
                {
                  char c = buffer[i];
                  if (c == '\n')
                    {
                      char newline[2] = {'\r', '\n'};
                      check_write = write(1, newline, 2);
                      if (check_write < 0)
                        {
                          fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
                          tcsetattr(0,TCSANOW,&original_Attributes);
                          exit(1);
                        }
                    }
                  else
                    {
                      check_write = write(1,&c, 1);
                      if (check_write < 0)
                        {
                          fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
                          tcsetattr(0,TCSANOW,&original_Attributes);
                          exit(1);
                        }
                    }

                }
		}
	    }
	} //end poll

    } //end while
}

/*Chcek for errors writing to the log */
void check_log(int log_check)
{
  if (log_check < 0)
    {
      fprintf(stderr, "Error writing to the log file: %s\n", strerror(errno));
      tcsetattr(0,TCSANOW,&global_attributes); 
      exit(1);
    }
}

/*Writes to the log file */
void write_to_log(int sending, int value) { 

  int check_log_file;

  if (sending == 1)
    check_log_file = write(log_file,"SENT ", 5);
  else
    check_log_file = write(log_file, "RECEIVED ", 9);

  check_log(check_log_file);

  char number[10];
  sprintf(number, "%d", value);

  check_log_file = write(log_file,number, strlen(number));
  check_log(check_log_file);

  check_log_file = write(log_file, " bytes: ", 8);
  check_log(check_log_file);

  check_log_file = write(log_file, buffer, value);
  check_log(check_log_file);

  check_log_file = write(log_file, "\n", 1);
  check_log(check_log_file);


}

int main(int argc, char* argv[]){



  /* Saves the original terminal properties */
  struct termios original_attributes; //original terminal mode
  tcgetattr(0,&original_attributes);
  tcgetattr(0,&global_attributes);
  /*----------------------------*/


  /* Setting up values for z_stream structures */

  out_stream.zalloc = Z_NULL;
  out_stream.zfree = Z_NULL;
  out_stream.opaque = Z_NULL;

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
  
  /*========================================== */


  int opt;
  int has_compress = 0; //checks if compressed argument is passed in
  int port;
  int gave_port = 0;
  struct sockaddr_in server_address;
  int sock = 0;
  char * file = NULL;
  memset(&server_address, '0', sizeof(server_address));


  /* This part of code handles the getopt parsing of optional argument */
  /* ================================================================= */

  struct option client_args[] =
    {
      {"log", required_argument, NULL, 'l' },
      {"port", required_argument, NULL, 'p' },
      {"compress", no_argument, &has_compress, 1},
      {0,0,0,0}
    };


/*Parses the arguments inputted through the command line. */
  while ((opt = getopt_long(argc,argv, "", client_args, NULL )) != -1)
    {
      switch(opt)
	{
	case 0:
	  break;
	case 'l':
	  is_log = 1;
	  file = optarg;
	  break;
	case 'p':
	  gave_port = 1;
	  port = atoi(optarg);
	  break;
	case '?':
	  fprintf(stderr, "Error, unknown input argument, '%s'.\n", argv[optind-1]);
	  exit(1);
	}

    }

  //No port was given
  if (gave_port == 0)
    {
      fprintf(stderr, "Error, no port was given, exiting...\n");
      exit(1);
    }

  if (is_log == 1)
    {
      /* Limits the size of the file*/
      struct rlimit limit;
      limit.rlim_cur = 10000;
      limit.rlim_max = 10000;
      /*============================ */
      if (setrlimit(RLIMIT_FSIZE, &limit) < 0)
        {
          fprintf(stderr, "Error setting file limit: %s\n", strerror(errno));
          exit(1);
        }
      log_file = creat(file,0666);
      if (log_file < 0)
        {
          fprintf(stderr, "Error creating file: %s\n\r", strerror(errno));
          exit(1);
        }
      int check_access = access(file,W_OK);
      if (check_access < 0)
	{
	  fprintf(stderr,"Error accessing log file: %s\n", strerror(errno));
	  exit(1);
	}
    }


  /* End of part of code that handles the parsing of argument */
  /* ======================================================== */


  /* Initializing network setting */
  /* ======================================================== */
  
  sock = create_socket(original_attributes);
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
    {
      fprintf(stderr, "Invalid address, exiting...\n");
      tcsetattr(0,TCSANOW,&original_attributes);
      exit(1);

    }

  if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
      fprintf(stderr, "Error while connecting, exiting..\n");
      tcsetattr(0,TCSANOW,&original_attributes);
      exit(1);
    }


    /* Starting transferring to the server */
    /*=====================================*/

  set_attributes(); //sets non-canonical mode for the terminal
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    fds[1].fd = sock;
    fds[1].events = POLLIN;

    write_to_server(sock, original_attributes, fds, has_compress);

    deflateEnd(&out_stream);
    inflateEnd(&in_stream);

    tcsetattr(0,TCSANOW,&original_attributes);
    exit(0);

}
