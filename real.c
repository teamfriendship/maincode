#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define MY_MSGQ_KEY 8101

void time_tick(int);
void do_io();
void schedule();
void printf_queue();
int *msgq_id;
int global_tick = 0;
int timetick = 10;
pcb* proc;

int current;
enum proc_state {
	READY,
	WAIT,
};

typedef struct {
	int pid;
	enum proc_state state;
	int cpu_burst;
	int io_burst;
	int turn;
}pcb;

typedef struct {
	long type;
	int time_quantum;
}message;

typedef struct qNode
{
	pcb* data;
	struct qNode *next;

}QUEUE_NODE;

typedef struct
{
	QUEUE_NODE *front;
	QUEUE_NODE *rear;
	int count;
} QUEUE;


QUEUE *createQueue()
{
	QUEUE *queue;
	queue = (QUEUE*)malloc(sizeof(QUEUE));
	if (queue)
	{
		queue->front = NULL;
		queue->rear = NULL;
		queue->count = 0;
	}
	return queue;
}


int enqueue(QUEUE *queue, pcb* item)
{
	QUEUE_NODE *newPtr;


	if (!(newPtr = (QUEUE_NODE*)malloc(sizeof(QUEUE_NODE))))
		return 0;


	newPtr->data = item;
	newPtr->next = NULL;


	if (queue->count == 0)
		queue->front = newPtr;
	else
		queue->rear->next = newPtr;

	(queue->count)++;
	queue->rear = newPtr;
	return 1;
}

int run_enqueue(QUEUE *queue, pcb* item)
{
	QUEUE_NODE *newPtr;

	if (!(newPtr = (QUEUE_NODE*)malloc(sizeof(QUEUE_NODE))))
		return 0;

	newPtr->data = item;
	newPtr->next = queue->front;
	queue->front = newPtr;
	(queue->count)++;
	return 1;
}

pcb* dequeue(QUEUE *queue)
{
	QUEUE_NODE *deleteLoc;

	if (!queue->count)
		return NULL;

	pcb* item = queue->front->data;

	deleteLoc = queue->front;

	if (queue->count == 1)
		queue->rear = queue->front = NULL;
	else
		queue->front = queue->front->next;

	(queue->count)--;
	free(deleteLoc);

	return item;
}

int emptyQueue(QUEUE *queue)
{
	return (queue->count == 0);
}

int queueCount(QUEUE *queue)
{
	return queue->count;
}

QUEUE *destroyQueue(QUEUE *queue)
{
	QUEUE_NODE *deletePtr;


	if (queue)
	{
		while (queue->front != NULL)
		{
			deletePtr = queue->front;
			queue->front = queue->front->next;
			free(deletePtr);
		}
		free(queue);
	}
	return NULL;
}

QUEUE* run_q;
QUEUE* wait_q;

void time_tick(int signo)
{
	
}


void do_cpu()
{
	proc->cpu_burst--;
	if(proc->cpu_burst>0)
	{
		
	}
	else
	{
		
	}
}

void do_io()
{
	
}

void schedule()
{
	
}

void printf_queue()
{
	
}

int main(int argc, char *argv[])
{
	run_q = createQueue();
	wait_q = createQueue();

	int i = 0;
	int pid[10] = {0,}
	struct sigaction par_sighandler, chi_sighandler;
	struct itimerval timer;
	int ret;
	message msg;
	srand((unsigned)time(NULL));

	for (i = 0; i < 10; i++) {
		pid[i] = fork();
		if (pid[i] == 0) {
			
			printf("child: pid[%d] = %d\n", i, pid);
                        proc = (pcb*)malloc(sizeof(pcb));
                        memset(proc, 0, sizeof(pcb));
                        proc->pid = pid;
                        proc->state = READY;
                        proc->cpu_burst = rand() % 80;
                        proc->io_burst = rand() % 5;
                        proc->turn = 0;
			printf("cpu time get : %d\n", proc->cpu_burst);
                        printf("io time get : %d\n", proc->io_burst);
			
			cpu_sighandler.sa_handler = &do_cpu;
		        sigemptyset(&cpu_sighandler.sa_mask);
			sigaction(SIGUSR1, &cpu_sighandler, 0);
			while (1)
			{
			}
			return 0;
		}
		else if (pid < 0) {
			printf("fork failed \n");
			return 0;
		}
		else {
			enqueue(run_q, pid[i]);
		}
	}

	new_sighandler.sa_handler = &time_tick;
	sigemptyset(&new_sighandler.sa_mask);
	sigaction(SIGALRM, &new_sighandler, 0);

	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	global_tick = 0;
	setitimer(ITIMER_REAL, &timer, NULL);

	msgq_id = (int *)malloc(sizeof(int));
	ret = msgget(MY_MSGQ_KEY, IPC_CREAT | 0644);
	if (ret == -1) {
		printf("msgq creation error ! (p) ");
		printf("errno: %d\n", errno);
		*msgq_id = -1;
		exit(0);
	}
	else {
		*msgq_id = ret;
		printf("msgq (key:0x%x, id:0x%x) created.\n", MY_MSGQ_KEY, ret);
	}

	while (1)
	{
		//printf_queue();
		if ((ret = msgrcv(*msgq_id, &msg, (sizeof(msg) - sizeof(long)), current, 0))==-1)
		{
			continue;
		}
		else
		{
			enqueue(wait_q, dequeue(run_q));
			//kill(run_q->front->data->pid, SIGUSR1);
			timetick=10;
		}
	}
}

