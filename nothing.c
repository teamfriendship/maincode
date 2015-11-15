#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

void sigalarm(int signo)
{
	struct timeval tm;

	gettimeofday(&tm, NULL);
	printf("sigalarm() called %d, time[%d,%d]\n",signo, tm.tv_sec, tm.tv_usec);
}

int main()
{
	int k;
	struct itimerval ival, oval;
	struct sigaction si, so;
	
	memset(&si, 0x00, sizeof(si));
	memset(&so, 0x00, sizeof(so));
	si.sa_handler = sigalarm;
	sigaction(SIGALRM, &si, &so);

	memset(&ival, 0x00, sizeof(ival));
	memset(&oval, 0x00, sizeof(oval));

	ival.it_interval.tv_usec = 100000;
	ival.it_value.tv_usec = 1;
	setitimer(ITIMER_REAL, &ival, &oval);

	printf("This is start Test time [%d]\n",time(NULL));
	for(k=0;k<10;k++)
	{
		printf("Test time [%d]\n", time(NULL));
		sleep(1);
	}
	printf("This is last Test time [%d]\n", time(NULL));
}
