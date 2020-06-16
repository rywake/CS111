
#include <math.h>
#include <poll.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h> 
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFF_SIZE 256

char buffer[BUFF_SIZE] = {0}; //buffer that holds contents read from stdin
char curr_command[BUFF_SIZE] = {0}; //buffer that hold the current command that will be processed when newline received
int curr_pos = 0; //current position in curr_command buffer to insert new character
int print_temp = 1;
int is_celcius = 0;
int is_log = 0;
sig_atomic_t volatile run_flag = 1;
int period_sec = 1;
struct tm * time_info;
time_t curr_time;
int log_fd;
time_t start_time;
double temp_conversion(double temp);
void write_shutdown();

void do_when_interrupted() {
  run_flag = 0;
  time(&curr_time);
  time_info = localtime(&curr_time);
  write_shutdown(); 
}

void write_shutdown() {
   char time[9];

   sprintf(time, "%02d", time_info->tm_hour);
   time[2] = ':';
   sprintf(time+3,"%02d", time_info->tm_min);
   time[5] = ':';
   sprintf(time+6, "%02d", time_info->tm_sec);
   time[8] = ' ';
   if (is_log)
     {
        write(log_fd, time, 9);
        write(log_fd, "SHUTDOWN\n", 9);
     }
   if (print_temp)
    { 
       write(1, time, 9);   
       write(1, "SHUTDOWN\n", 9);
    }
   exit(0);
}

/* Checks time between last written and current time */
double check_timer() {
time_t new_time = time(&new_time);
double difference = difftime(new_time, start_time);

return difference;
}

/* Writes time and temp to logfile */
void write_temperature_file(double therm_val) {
char time[9];

sprintf(time, "%02d", time_info->tm_hour);
time[2] = ':';
sprintf(time+3,"%02d", time_info->tm_min);
time[5] = ':';
sprintf(time+6, "%02d", time_info->tm_sec);
time[8] = ' ';
write(log_fd, time, 9);

char temp[5];
sprintf(temp, "%.1f", temp_conversion(therm_val));
temp[4] = '\n';
write(log_fd, temp, 5);

}

/* Converts sensor output to temperature */
double temp_conversion(double temp) {
  const int B = 4275;
  const float R0 = 100000;
  double R = R0*(1023.0/temp-1.0); 
  double celcius = 1.0/(log(R/R0)/B+1/298.15)-273.15;
  if (is_celcius)
    {
      return celcius; 
    }
  double fahrenheit = (9.0/5.0)*celcius + 32;

  return fahrenheit;
}


/* Executes command given from stdin */
void execute_command(char * command) {
  char period[8] = {'\n'};
  char log[5] = {'\n'};
  memcpy(log,command, 4);
  memcpy(period, command, 7);
  if (strcmp("SCALE=F", command) == 0)
    {
      is_celcius = 0;
      if (is_log)
	write(log_fd, "SCALE=F\n", 8);
    }
  else if (strcmp("SCALE=C", command) == 0)
    {
      is_celcius = 1;
      if (is_log)
	write(log_fd, "SCALE=C\n", 8);
    }
  else if (strcmp("STOP", command) == 0)
    {
      print_temp = 0;
      if (is_log)
	write(log_fd, "STOP\n", 5);
    }
  else if (strcmp("START", command) == 0)
    {
      print_temp = 1;
      if (is_log)
	write(log_fd,"START\n", 6);
    }
  else if (strcmp("OFF", command) == 0)
    {
      if (is_log)
	write(log_fd,"OFF\n",4);
      do_when_interrupted();
    }
  else if (strcmp("LOG ", log) == 0)
    {
      if (is_log)
	{
	  write(log_fd, command, strlen(command));
          write(log_fd, "\n", 1);
	}
	
    }
  else if (strcmp("PERIOD=", period) == 0)
    {
       int length = strlen(command);
       int j;
       char value[BUFF_SIZE] = {'\n'};
       for (j =7; j < length; j++)
	{
	   value[j-7] = command[j];
        }
	if (atoi(value) >= 0)
            period_sec = atoi(value);
       if (is_log && atoi(value) >= 0)
	{
	 write(log_fd, command, strlen(command));
         write(log_fd, "\n", 1);
        }
     }

}

/* Stores data read from stdin to a buffer */
void process_input() {
  int check = read(0,buffer,BUFF_SIZE);
  int i;
  if (check > 0)
    {
      for (i = 0; i < check; i++)
	{
	  char c = buffer[i];
	  if (c == '\n')
	    {
	      curr_command[curr_pos] = '\0';
	      execute_command(curr_command);
	      curr_pos = 0;
	    }
	  else
	    {
	      curr_command[curr_pos] = c;
	      curr_pos++;
	    }
	}

    }

}

void check_error_poll(int check_error)
{
  if (check_error < 0)
    {
      fprintf(stderr, "Error running poll: %s\n", strerror(errno));
      exit(1);
    }

}

int main(int argc, char* argv[]) {

  
struct pollfd Input;
Input.fd = 0;
Input.events = POLLIN;
int check_poll; //Checks for errors in the poll  
int opt;
  struct option optional_arg[] =
    {
      {"scale",required_argument,NULL, 's' },
      {"log", required_argument, NULL, 'l'},
      {"period", required_argument, NULL, 'p'},
      {0,0,0,0}
    };

  while ((opt = getopt_long(argc,argv, "", optional_arg, NULL)) != -1)
   {
     switch(opt)
       {
       case 's':
	if (strcmp(optarg,"C") == 0)
	   {
	     is_celcius = 1; 
	   }
	else if (strcmp(optarg,"F") == 0)
	  {
	     is_celcius = 0;
	  }
	else
	{
	   fprintf(stderr, "Invalid option for '--scale', exiting...\n");
	   exit(1);  
	}
	 break;
       case 'l':
	is_log = 1;
	log_fd = creat(optarg,0666);
	 if (log_fd < 0)
	     {
	       fprintf(stderr, "Error, unable to create/open input file '%s': %s\n", optarg, strerror(2));
	       exit(1);
	     }
	 break;
       case 'p':
	 period_sec = atoi(optarg);
	 break;
       case '?':
         fprintf(stderr, "Error, unknown input argument, '%s'.\n",argv[optind-1]);
         exit(1);
       }
   }

  double thermo_value;
  mraa_gpio_context button;
  mraa_aio_context thermo;
  thermo = mraa_aio_init(1);
  button = mraa_gpio_init(60);

  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted,NULL);


start_time = time(&start_time);
if (print_temp)
     {
       thermo_value = mraa_aio_read(thermo);
       time(&curr_time);
       time_info = localtime(&curr_time);
       printf("%02d:%02d:%02d %.1f\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec,temp_conversion(thermo_value));
       if (is_log)
         write_temperature_file(thermo_value);
     }

  while (run_flag) {
    if (check_timer() >= period_sec)
   {
    start_time = time(&start_time);
    if (print_temp)
     {
       thermo_value = mraa_aio_read(thermo);
       time(&curr_time);
       time_info = localtime(&curr_time);   
       printf("%02d:%02d:%02d %.1f\n",time_info->tm_hour,time_info->tm_min,time_info->tm_sec,temp_conversion(thermo_value));
       if (is_log)
	 write_temperature_file(thermo_value);
     }
   }
    check_poll = poll(&Input, 1, 0);
    check_error_poll(check_poll); 
   if (check_poll > 0)
      process_input();
    //sleep(period_sec);

  }

  mraa_aio_close(thermo);
  mraa_gpio_close(button);


  return 0;
}
