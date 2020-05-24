#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <resolv.h>

/*
gcc daemon.c -o daemon -lpthread
./daemon cmd.txt
kill -SIGINT pid
*/

bool flag = false;
bool termination = false;
sem_t semaphore;

void catch_sigint(int signum) {
	flag = true;
}

void catch_sigterm(int signum) {
	termination = true;
}

void catch_sigchld(int signum) {
	wait(NULL);
	sem_post(&semaphore);
}

void write_message(const char* message) {
	int logfile = open("log.txt", O_CREAT | O_RDWR, S_IRWXU);
	lseek(logfile, 0, SEEK_END);
	write(logfile, message, strlen(message));
	close(logfile);
}

int Daemon(char* filename) {
	signal(SIGINT, catch_sigint);
	signal(SIGTERM, catch_sigterm);
	signal(SIGCHLD, catch_sigchld);

	openlog("Daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_NOTICE, "Daemon is working...");
	write_message("Daemon starts its work\n");
	sem_init(&semaphore, 0, 1);

	while (1) {
		pause();
		if (flag) {
			flag = false;
			syslog(LOG_NOTICE, "Signal by user is catched");
			write_message("Signal by user is catched\n");
			char buf[1024];
			int fd = open(filename, O_CREAT | O_RDWR, S_IRWXU);
			read(fd, buf, sizeof(buf));
			close(fd);

			int output = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
			// parse commands
			char *commands[124];
			int command_number = 0;
			char *ptr = strtok(buf, "\n");
			while (ptr != NULL) {
				commands[command_number] = ptr;
				commands[command_number++][strlen(ptr)] = '\0';
				ptr = strtok(NULL, "\n");
			}
			commands[command_number] = NULL;

			pid_t pid;

			for (int i = 0; i < command_number - 1; i++) {
				pid = fork();
				if (pid == 0) {
					//parse arguments in every command
					char* arg_list[100];
					int arg_number = 0;
					ptr = strtok(commands[i], " ");
					while (ptr != NULL) {
						arg_list[arg_number++] = ptr;
						ptr = strtok(NULL, " ");
					}
					arg_list[arg_number] = NULL;

					int wait = sem_wait(&semaphore);
					if (wait == -1) {
						syslog(LOG_WARNING, "Error occured while locking semaphore");
						exit(EXIT_FAILURE);
					}
					char message[32];
					sprintf(message, "Daemon executes command %d\n", i + 1);
					write_message(message);
					close(1);
					dup2(output, 1);
					if (execve(arg_list[0], arg_list, NULL) == -1) {
						write_message("Execution of command failed\n");
						exit(EXIT_FAILURE);
					}
				}
				else if (pid != 0) { // parent
					while (1) {
						pause();
						break;
					}
				}
			}
			close(output);
		}
		if (termination) {
			syslog(LOG_NOTICE, "Daemon is terminated");
			closelog();
			write_message("Daemon finished its work");
			sem_destroy(&semaphore);
			exit(EXIT_SUCCESS);
		}
	}
}

int main(int argc, char* argv[]) {
	pid_t pid;
	pid = fork();

	if (pid == -1) {
		printf("Can't fork, error occured\n");
		exit(EXIT_FAILURE);
	}
	else if (pid != 0) { // parent
		exit(EXIT_SUCCESS);
	}
	setsid(); 	// PID = 0, child
	printf("My daemon is started. PID = %i\n", getpid());
	int status = Daemon(argv[1]);
	printf("My daemon is finished. Return code is %i", status);
	return 0;
}
