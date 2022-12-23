# ARP-Hoist-Assignment
Deliverable by Miguel Angel Sempere Vicente and David Villanueva Álvarez.

#Github link
https://github.com/scoobygalleta/ARP_1
# Description of the project and deployed

The hoist shown on the inspection console must be able to be moved to the different points of the two-dimensional space with lower (0,0) and upper (39,10) limits.

This can be moved or stopped from the input by means of command console buttons.
This requires a series of communications realized by writing and reading in pipes:

 -- The command console reads the commands entered by the user and increments the velocities. These velocities are written on the file descriptor only when a button is pressed.  They are written in two different named pipes, depending on the motor destination.

 -- The motors have the task of bringing the hoist to the position indicated by the speed set by the command console. To do so, they read the speed and calculate the position.

 -- As the motors may have some error, it is necessary to know the actual position of the hoist. For this purpose the actual position of the problem is determined by means of the world file. It determines this position by reading the wrong position of the motors from a pipe, calculating the  real position by adding the error of the motors, and writing it to another pipe to send it to the inspection console.

 -- As a last step, the inspection console opens the pipe that has the actual value of the position, reads it and plots it in the graphical window.

-- Each time a button is pressed a new line is writen onto the logfile of that specific process along with a timestamp. By doing so, it is easy to keep track of the actions that took place during execution.

-- In the watchdog, inotify is used to acknowledge the new changes in any logfile. Each time a change is detected, the timer is reset. If the timer is over, a signal SIGTERM is sent to each process in order to close everything properly (fd, files, FIFO).
 
In addition, the inspection screen has two buttons, Stop and Reset, with the following functions:
 -- STOP: It stop the hoist. Motors has to put velocity in 0 directly, without decreasing, and when the user try to move the hoist, it could be moved agan.
 -- RESET: It rewinds to the (0,0) position with -1 velocity. It has 2 additional instructions: is not affected in its execution by the speeds included by the user through the command console and when the stop button is pressed, its execution is stopped.
 
# Who to build and execute
To create and execute the files it is only necessary to execute the following code inside the console the command:
 >> ./hoist.sh

# User Guide

If weird behaviors happen after launching the application (buttons not spawning, crash in the first execution) simply try to resize the terminal window or relaunch the program.
