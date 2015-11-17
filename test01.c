#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int count;

void sigint_handler(int signum){
	//int count = 0;
	printf("timer expired  %d\n times", ++count);
}

int main(){
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sigint_handler;
	sigaction(SIGALRM, &sa, NULL);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 10000;

	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 10000;

	setitimer(ITIMER_REAL, &timer, NULL);
	while(1);
	count++;
}

