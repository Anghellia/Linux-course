#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "syslog.h"
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* 
gcc daemon.c -o daemon
./daemon cmd.txt
kill -SIGINT pid
*/

bool flag = false;
bool termination = false;

void catch_sigint(int signum) {
	flag = true;
}

void catch_sigterm(int signum) { 
	termination = true;
}

int Daemon(char* filename) {
	signal(SIGINT, catch_sigint);
	signal(SIGTERM, catch_sigterm);

	openlog("Daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_NOTICE, "Daemon is working...");

	while (1) {
		pause();
		if (flag) {
			flag = false;
			syslog(LOG_NOTICE, "Signal by user is catched");

			pid_t pid;
			pid = fork();

			if (pid == 0) {
				char buf[1000];
				int command = open(filename, O_CREAT | O_RDWR, S_IRWXU);
				int length = read(command, buf, sizeof(buf));
				close(command);
				buf[length - 1] = '\0';

				int index = 0;
				char *arguments[10];
				char *cmd = strtok(buf, " ");
				arguments[index++] = cmd;

				while (cmd != NULL) {
					cmd = strtok(NULL, " ");
					arguments[index++] = cmd;
				}
				arguments[index] = NULL;

				int output = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
				lseek(output, 0, SEEK_END);
				close(1);
				dup2(output, 1);

				execve(arguments[0], arguments, NULL);
			}
			else {
				int status;
				wait(&status);
			}
		}
		if (termination) {
			syslog(LOG_NOTICE, "Daemon is terminated");
			closelog();
			exit(EXIT_SUCCESS);
		}
	}
}

int main(int argc, char* argv[]) {
	pid_t pid;
	pid = fork();

	if (pid == -1) {
		exit(EXIT_FAILURE);
	}
	else if (pid != 0) {
		exit(EXIT_SUCCESS);
	}
	setsid(); 
	printf("My daemon is started. PID = %i\n", getpid());
	int status = Daemon(argv[1]);
	printf("My daemon is finished. Return code is %i", status);
	return 0;
}
