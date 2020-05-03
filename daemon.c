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

bool flag = false;

void catch_sigint(int signum) {
	flag = true;
}

int Daemon() {
	signal(SIGINT, catch_sigint);

	openlog("Daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_NOTICE, "Daemon is working...");

	while (1) {
		pause();
		if (flag) {
			flag = false;
			syslog(LOG_NOTICE, "Signal by user is catched");
			char buf[] = "It's a daemon process.";
			int fd = open("output.txt", O_CREAT | O_RDWR, S_IRWXU);
			lseek(fd, 0, SEEK_END);
			write(fd, buf, sizeof(buf) - 1);
			close(fd);
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
	pid = fork();
	
	if (pid == -1) {
		exit(EXIT_FAILURE);
	}
	else if (pid != 0) {
		exit(EXIT_SUCCESS);
	}
	printf("My daemon is started. PID = %i\n", getpid());
	int status = Daemon();  
	printf("My daemon is finished. Return code is %i", status);
	return 0;
}