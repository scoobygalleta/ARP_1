#include "./../include/command_utilities.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define SIZE 80

float SPEED_MIN = -2;
float SPEED_MAX = 2;

int fd_mx;
int fd_mz;
float v_inc = 0.25;

int stop=0;
int reset=0;

float v_x = 0;
float v_z = 0;

char msg[SIZE];
char line[SIZE];
char * cmd_mx;
char * cmd_mz;

FILE *get_pid;
FILE *log_file;

void check_limits(float *v1, float limit_min, float limit_max);

void stp(int sig) {

    if (sig == SIGUSR2) {
        // Getting the pid of process "command"
        v_x = 0.0;
        v_z = 0.0;

        sprintf(msg, "%f", v_x);
        int wr_x = write(fd_mx, msg, SIZE);
        if (wr_x==0){
            perror ("Cannot write in fifo");
        }

        sprintf(msg, "%f", v_x);
        int wr_z = write(fd_mz, msg, SIZE);
        if (wr_z==0){
            perror ("Cannot write in fifo");
        }
        reset=0;
    }
}
void rst(int sig){
    if (sig==SIGUSR1){
        v_x= -1;
        v_z= -1;
        reset=1;
    }
}


void exit_handler(int sig) {
    // Signal from the watchdog
    if (sig == SIGTERM) {
        close(fd_mx);
        unlink(cmd_mx);
        close(fd_mz);
        unlink(cmd_mz);
        fclose(log_file); 
        exit(EXIT_SUCCESS);
    }
}

void check_limits(float *v1, float limit_min, float limit_max) {

    // Check if speed is in the limits
    if (*v1 <= limit_min)
        *v1 = limit_min;
    else if (*v1 >= limit_max)
        *v1 = limit_max;

}

int main(int argc, char const *argv[]) {

    // Initialize User Interface 
    init_console_ui();

    // Signal from the inspection console (Stop)
    if (signal(SIGUSR2, stp)== SIG_ERR)
        printf("\ncan't catch SIGUSR2\n");
    
    if (signal(SIGUSR1,rst)==SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");
        
    // Signal from the watchdog (Terminate)
    if (signal(SIGTERM, exit_handler) == SIG_ERR)
        printf("\ncan't catch SIGTERM\n");

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Creating the logfile
    log_file = fopen("./log/command_console.log", "w");
    if (log_file == NULL) { /* Something is wrong   */}

    // Pipes 
    cmd_mx = "/tmp/cmd_mx"; 
    mkfifo(cmd_mx, 0666);
    cmd_mz = "/tmp/cmd_mz"; 
    mkfifo(cmd_mz, 0666);

    // Pipe open
    fd_mx = open(cmd_mx, O_WRONLY); 
    if (fd_mx == 0){
        perror("Cannot open fifo");
    }
    fd_mz = open(cmd_mz, O_WRONLY);
    if (fd_mz == 0){
        perror("Cannot open fifo");
    }
    
    // Infinite loop
    while(TRUE) {

        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

        // Get the current time to write on the logfile
        time_t clk = time(NULL);
        char * timestamp = ctime(&clk);
        timestamp[24] = ' ';

        // If user resizes screen, re-draw UI
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }
        // Else if mouse has been pressed
        else if(cmd == KEY_MOUSE) {

            // Check which button has been pressed...
            if(getmouse(&event) == OK) {

                // Vx-- button pressed
                if(check_button_pressed(vx_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Decreased");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }   

                    fprintf(log_file,"%s- Vx-- Pressed: Horizontal Speed Decreased\n", timestamp);
                    fflush(log_file); 
                    if (reset==0){
                        v_x -= v_inc; 
                    }
                    check_limits(&v_x, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_x);
                    int wr_x = write(fd_mx, msg, SIZE);
                    if (wr_x==0){
                        perror ("Cannot write in fifo");
                    }

                }

                // Vx++ button pressed
                else if(check_button_pressed(vx_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Increased");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }

                    fprintf(log_file,"%s- Vx++ Pressed: Horizontal Speed Increased\n", timestamp);
                    fflush(log_file); 
                    if (reset==0){
                        v_x += v_inc;
                    }
                    check_limits(&v_x, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_x);
                    int wr_x = write(fd_mx, msg, SIZE);
                    if (wr_x==0){
                        perror ("Cannot write in fifo");
                    }
                }

                // Vx stop button pressed
                else if(check_button_pressed(vx_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Motor Stopped");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }

                    fprintf(log_file,"%s- Vx STP Pressed: Horizontal Motor Stopped\n", timestamp);
                    fflush(log_file); 
                    if (reset==0){
                        v_x = 0;
                    }
                    //check_limits(&v_x, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_x);
                    int wr_x = write(fd_mx, msg, SIZE);
                    if (wr_x==0){
                        perror ("Cannot write in fifo");
                    }
                }

                // Vz-- button pressed
                else if(check_button_pressed(vz_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Decreased");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }

                    fprintf(log_file,"%s- Vz-- Pressed: Vertical Speed Decreased\n", timestamp);
                    fflush(log_file); 
                    if (reset==0) {
                        v_z -= v_inc;
                    }
                    check_limits(&v_z, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_z);
                    int wr_z = write(fd_mz, msg, SIZE);
                    if (wr_z==0){
                        perror ("Cannot write in fifo");
                    }
                }

                // Vz++ button pressed
                else if(check_button_pressed(vz_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Increased");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }

                    fprintf(log_file,"%s- Vz++ Pressed: Vertical Speed Increased\n", timestamp);
                    fflush(log_file); 
                    if (reset==0){
                        v_z += v_inc;
                    }
                    check_limits(&v_z, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_z);
                    int wr_z = write(fd_mz, msg, SIZE);
                    if (wr_z==0){
                        perror ("Cannot write in fifo");
                    }
                }

                // Vz stop button pressed
                else if(check_button_pressed(vz_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Motor Stopped");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }

                    fprintf(log_file,"%s- Vz STP Pressed: Vertical Motor Stopped\n", timestamp);
                    fflush(log_file); 
                    if (reset==0){
                        v_z = 0;
                    }
                    //check_limits(&v_z, SPEED_MIN, SPEED_MAX);

                    sprintf(msg, "%f", v_z);
                    int wr_z = write(fd_mz, msg, SIZE);
                    if (wr_z==0){
                        perror ("Cannot write in fifo");
                    }
                }
            }
        }

        refresh();
	}

    // Close PIPE
    close(fd_mx); 
    close(fd_mz);
    fclose(log_file);

    // Terminate
    endwin();
    return 0;
}
