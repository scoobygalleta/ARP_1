#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define SIZE 80

#define X_MIN 0
#define X_MAX 39

float SPEED_MIN = -2;
float SPEED_MAX = 2;

float v_x = 0;
float freq = 1;

int update = 0;

// File descriptors
int fd_cmd, fd_world;

// End-effector coordinates
float ee_x = 0;

char command_c[SIZE];
char command_x[SIZE];
char * mx_world;

FILE *log_file;


void rst(int sig) {
    // Signal from the inspection console (Reset)
    if (sig == SIGUSR2) {
            v_x=-1;
    }
}


void exit_handler(int sig) {
    // Signal from the watchdog
    if (sig == SIGTERM) {
        close(fd_cmd);
        close(fd_world);
        unlink(mx_world); 
        fclose(log_file); 
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {

    // Signal from the inspection console (Reset)
    if (signal(SIGUSR2, rst) == SIG_ERR)
        printf("\ncan't catch SIGUSR2\n");

    // Signal from the watchdog (Terminate)
    if (signal(SIGTERM, exit_handler) == SIG_ERR)
        printf("\ncan't catch SIGTERM\n");

    fd_set readfds;
    int retval;
    struct timeval tv;

    // Creating the logfile
    log_file = fopen("./log/m_x.log", "w");
    if (log_file == NULL) { /* Something is wrong   */}

    // Pipes
    char * cmd_mx = "/tmp/cmd_mx";
    mx_world = "/tmp/mx_world"; 
    mkfifo(mx_world, 0666);

    while (1) {

        // Get the current time to write on the logfile
        time_t clk = time(NULL);
        char * timestamp = ctime(&clk);
        timestamp[24] = ' ';

        // Pipe open
        fd_cmd = open(cmd_mx, O_RDONLY);
        

        float ee_x_prev = ee_x;

        // Initialize with an empty set the file descriptors set
        FD_ZERO(&readfds);
        // Add the file descriptor to the file descriptors set
        FD_SET(fd_cmd, &readfds);

        tv.tv_sec = freq;

        retval = select(fd_cmd+1, &readfds, NULL, NULL, &tv);

        switch (retval) {
            case -1:
                perror("select()");
                break;
            case 1: ;
                int rd_cmd =read(fd_cmd, command_c, SIZE);
                if (rd_cmd == 0){
                    perror("Cannot read fifo.");
                }

                v_x = atof(command_c);
                close(fd_cmd);

                break;
        }    

        if(v_x != 0 ) { // || reset == 1
            ee_x += v_x;
            // enviar señal a watchdog cuando hay un cambio?
        }
        else { // v_x = 0

        }
        
        if (ee_x < X_MIN) {
            ee_x = X_MIN;
        }  
        else if (ee_x > X_MAX) {
            ee_x = X_MAX;   
        }
        
        printf("MX = %f \n", ee_x); fflush(stdout);

        // We only send to world when we update the position
        if (ee_x_prev != ee_x) {
            sprintf(command_x, "%f", ee_x);
            fd_world = open(mx_world, O_WRONLY);
            int wr_world = write(fd_world, command_x, SIZE);
            if (wr_world==0){
                perror ("Cannot write in fifo");
            }

        }

        close(fd_cmd);
        close(fd_world);
    }

    fclose(log_file); 

    return 0;

}
