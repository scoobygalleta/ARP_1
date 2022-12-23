#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include<sys/inotify.h>
#include<fcntl.h> 

#define TIMER 60

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int timer = TIMER;

pid_t pid_cmd, pid_insp, pid_mx, pid_mz, pid_world, pid_master;

int fd, wd;

void kill_all(int sig);

void sig_handler(int sig) {

	if (sig == SIGINT) {
		// Remove the file descriptor and close the inotify instance
		inotify_rm_watch(fd, wd);
		close(fd);
		exit(EXIT_SUCCESS);
	}
		
}

void kill_all(int sig) {

	// Send signal to the Command console
	kill(pid_cmd, sig);
	// Send signal to the Inspection console
	kill(pid_insp, sig);
	// Send signal to the Motor x 
	kill(pid_mx, sig);
	// Send signal to the Motor z
	kill(pid_mz, sig);
	// Send signal to World
	kill(pid_world, sig);
	// Kill master (own process)
	kill(pid_master, sig);
				
}


int spawn(const char * program, char * arg_list[]) {

	pid_t child_pid = fork();

	if(child_pid < 0) {
		perror("Error while creating fork");
		return 1;
	}
	if (child_pid == 0) {
		execvp(arg_list[0], arg_list);
		perror("exec failed");
		return 1;
	}
	else {
		return child_pid;int status;
	}
}

int main() {

	int status;

	char * arg_list_command[] = { "/usr/bin/konsole", "-e", "./bin/command", NULL };
	char * arg_list_inspection[] = { "/usr/bin/konsole", "-e", "./bin/inspection", NULL };
	char * arg_list_mx[] = { "./bin/m_x", NULL };
	char * arg_list_mz[] = { "./bin/m_z", NULL };
	char * arg_list_world[] = { "./bin/world", NULL };

	pid_cmd = spawn("/usr/bin/konsole", arg_list_command);
	pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
	pid_mx = spawn("./bin/m_x", arg_list_mx);
	pid_mz = spawn("./bin/m_z", arg_list_mz);
	pid_world = spawn("./bin/world", arg_list_world);
	pid_master = getpid();

	// Initialize inotify
	fd = inotify_init();
	
	if(fd < 0)  perror("inotify_init");

	wd = inotify_add_watch(fd, "./log", IN_MODIFY | IN_CREATE);

	// When we read using the fd descriptor, the thread will not be blocked.
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) { perror("fcntl"); fflush(stdout);}

	if(wd < 0){ perror("inotify_add_watch"); fflush(stdout); } 
	else { printf("Whatchdog is running...\nTimer: %i [s]\n", TIMER); fflush(stdout); }

	while (1) {

		char buffer[EVENT_BUF_LEN];
		int length, i = 0;

		// Read buffer
		length = read(fd, buffer, EVENT_BUF_LEN);

		//if(length < 0) { /*Something wrong happened*/ perror ("read"); }

		if (length > 0) {
			timer = TIMER;
		}

		if(timer == 0) {
			timer = TIMER;
			kill_all(SIGTERM);     
		}
	
		sleep(1);

		if (timer % 1 == 0)
			printf("Timer: %i\n", timer);
		
		fflush(stdout);
		timer--;

	}

	waitpid(pid_cmd, &status, 0);
	waitpid(pid_insp, &status, 0);
	waitpid(pid_mx, &status, 0);
	waitpid(pid_mz, &status, 0);
	waitpid(pid_world, &status, 0);

	printf ("Main program exiting with status %d\n", status);
	
	return 0;
}

