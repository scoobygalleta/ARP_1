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

#define Z_MIN 0
#define Z_MAX 10

float SPEED_MIN = -2;
float SPEED_MAX = 2;

float v_z = 0;
float freq = 1;

int update = 0;
int reset=0;

// File descriptors
int fd_cmd, fd_world;

// End-effector coordinates
float ee_z = 0;


char command_c[SIZE];
char command_z[SIZE];
char * mz_world;

FILE *log_file;


void rst(int sig) {
    
    // Signal from the inspection console (Reset)
    if (sig == SIGUSR2) {
            v_z=-1;

    }

}

void exit_handler(int sig) {
    // Signal from the watchdog
    if (sig == SIGTERM) {
        close(fd_cmd);
        close(fd_world);
        unlink(mz_world); 
        fclose(log_file); 
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    

    // Signal from the inspection console (Reset)
    if (signal(SIGUSR2, rst) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");

    // Signal from the watchdog (Terminate)
    if (signal(SIGTERM, exit_handler) == SIG_ERR)
        printf("\ncan't catch SIGTERM\n");

    fd_set readfds;
    int retval;
    struct timeval tv;

    // Creating the logfile
    log_file = fopen("./log/m_z.log", "w");
    if (log_file == NULL) { /* Something is wrong   */}

    // Pipes
    char * cmd_mz = "/tmp/cmd_mz";
    mz_world = "/tmp/mz_world"; 
    mkfifo(mz_world, 0666);

    while (1) {

        // Get the current time to write on the logfile
        time_t clk = time(NULL);
        char * timestamp = ctime(&clk);
        timestamp[24] = ' ';

        // Pipe open
        fd_cmd = open(cmd_mz, O_RDONLY);   

        float ee_z_prev = ee_z;

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
                int rd_cmd = read(fd_cmd, command_c, SIZE);
                if (rd_cmd == 0){
                    perror("Cannot read fifo.");
                }
                v_z = atof(command_c);
                close(fd_cmd);

                break;
        }    

        if(v_z != 0 ) { // || reset == 1
            ee_z += v_z;
            // enviar señal a watchdog cuando hay un cambio?
        }
        else { // v_z = 0

        }
        
        if (ee_z < Z_MIN) {
            ee_z = Z_MIN;
        }  
        else if (ee_z > Z_MAX) {
            ee_z = Z_MAX;   
        }
        
        printf("MZ = %f \n", ee_z); fflush(stdout);

        // We only send to world when we update the position
        if (ee_z_prev != ee_z) {
            sprintf(command_z, "%f", ee_z);
            fd_world = open(mz_world, O_WRONLY);
            write(fd_world, command_z, SIZE);
            int wr_world = write(fd_world, command_z, SIZE);
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
