#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define SIZE 80
	
float ee_x_world = 0.0;
float ee_z_world = 0.0;

float freq = 2;

// Variable to know which motor has been updated; x (1) or z (2)
int update = 0;

// File descriptors
int fd_mx, fd_mz, fd_ins;

char command_mx[SIZE];
char command_mz[SIZE];
char real_pos[SIZE];
char * world_ins;

FILE *log_file;

void exit_handler(int sig) {
	// Signal from the watchdog
	if (sig == SIGTERM) {
		close(fd_mx);
		close(fd_mz);
		close(fd_ins);
		unlink(world_ins); 
		fclose(log_file); 
		exit(EXIT_SUCCESS);
	}
}

int max(int num1, int num2) {
	return (num1 > num2 ) ? num1 : num2;
}

int main() {

	fd_set readfds;
	int retval;
	struct timeval tv;

	// Signal from the watchdog (Terminate)
	if (signal(SIGTERM, exit_handler) == SIG_ERR)
		printf("\ncan't catch SIGTERM\n");

	// Creating the logfile
	log_file = fopen("./log/world.log", "w");
	if (log_file == NULL) { /* Something is wrong   */}

	// Pipes
	char * mx_world = "/tmp/mx_world";
	char * mz_world = "/tmp/mz_world";
	world_ins = "/tmp/world_ins";
	mkfifo(world_ins, 0666);

	while (1) {

		// Randomizing seed for random error generator
		srand(time(NULL));

		// Get the current time to write on the logfile
		time_t clk = time(NULL);
		char * timestamp = ctime(&clk);
		timestamp[24] = ' ';

		// Pipe open
		fd_mx = open(mx_world, O_RDWR);
		if (fd_mx==0){
			perror("Cannot open fifo");
		}
		fd_mz = open(mz_world, O_RDWR);
		if (fd_mz==0){
			perror("Cannot open fifo");
		}

		// Initialize with an empty set the file descriptors set
		FD_ZERO(&readfds);
		// Add the file descriptor to the file descriptors set
		FD_SET(fd_mx, &readfds);
		FD_SET(fd_mz, &readfds);

		tv.tv_sec = freq;

		retval = select(max(fd_mx, fd_mz)+1, &readfds, NULL, NULL, &tv);
		
		switch (retval) {
			case -1:
				perror("select()");
				return -1;

			case 2: ;

				int rand_number = (rand() % 2);
				
				if (rand_number == 0) {
					int rd_mx = read(fd_mx, &command_mx, SIZE);
					if (rd_mx == 0){
						perror("Cannot read fifo");
					}
					sscanf(command_mx, "%f", &ee_x_world);
					close(fd_mx);
					close(fd_mz); 
					update = 1;
				}
				else {
					int rd_mz = read(fd_mz, &command_mz, SIZE);
					if (rd_mz==0){
						perror("Cannot read fifo");
					}
					sscanf(command_mz, "%f", &ee_z_world);
					close(fd_mx);
					close(fd_mz);
					update = 2;
				}
				break;

			case 1: // Only one pipe

				if (FD_ISSET(fd_mx, &readfds) != 0) {
					int rd_mx = read(fd_mx, &command_mx, SIZE);
					if (rd_mx == 0){
						perror("Cannot read fifo");
					}
					//ee_x_world = atof(command_mx);
					sscanf(command_mx, "%f", &ee_x_world);
					close(fd_mx);
					close(fd_mz);
					update = 1;
				}
				else if (FD_ISSET(fd_mz, &readfds) != 0) {
					int rd_mz = read(fd_mz, &command_mz, SIZE);
					if (rd_mz==0){
						perror("Cannot read fifo");
					}
					//ee_z_world = atof(command_mz);
					sscanf(command_mz, "%f", &ee_z_world);
					close(fd_mx);
					close(fd_mz);
					update = 2;
				}
				break;
				
		}

		if (update != 0) {
			// Adding error of up to +-5 percent
			float random_error = -0.005 + (float)rand() / ((float)RAND_MAX/(0.005+0.005));
			
			if (update == 1) {
				//printf("error x = %f \n", random_error); fflush(stdout);
				ee_x_world += random_error*ee_x_world;
			}
			else if (update == 2) {
				//printf("error z = %f \n", random_error); fflush(stdout);
				ee_z_world += random_error*ee_z_world;
			}

			update = 0;

		}

		close(fd_mx);
		close(fd_mz);

		printf("MX_WORLD = %f \n", ee_x_world); fflush(stdout);
		printf("MZ_WORLD = %f \n", ee_z_world); fflush(stdout);
		
		fd_ins = open(world_ins,O_RDWR);
		if (fd_ins ==0){
			perror ("Cannot open fifo");
		}
		sprintf(real_pos,"%f,%f",ee_x_world,ee_z_world);
		int wrt_ins = write(fd_ins,real_pos,strlen(real_pos)+1);
		if (wrt_ins==0){
			perror("Cannot write in fifo");
		}

		close(fd_ins);
	
	}

	fclose(log_file);
	
	return 0;
}