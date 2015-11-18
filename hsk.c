#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/time.h>
#include <sched.h>
#include <string.h>

#define TOTALFORK 10
#define BUFF_SIZE 1024

typedef struct q_node {
	pid_t pid;
	struct q_node* next;
}node;
typedef struct QUEUE {
	int total_count;
	struct q_node* front;
	struct q_node* rear;
}hnode;
typedef struct msg_childtoparent {//child가 parent에게 보내는 메세지 구조체
	long mtype;
	pid_t pid;
	int io_time;
}msg;

hnode* run_q;
hnode* wait_q;
int io_time[10] = { 0, };
int global_time_tick;
int key_id;
FILE* output_file;
int time_quantum;
int* cpu_bust;
int* io_bust;
key_t* msg_id;

int enqueue(hnode* queue, pid_t new_p) {
	if (queue == NULL)
		return 0;
	else {
		node* new_node = (node*)malloc(sizeof(node));
		if (new_node == NULL)
			return 0;
		else {
			new_node->pid = new_p;
			new_node->next = NULL;
		}
		if (queue->total_count == 0)
			queue->front = new_node;
		else
			queue->rear->next = new_node;
		(queue->total_count)++;
		queue->rear = new_node;
		return 1;
	}
}

int run_enqueue(hnode* queue, pid_t new_p) {
        if (queue == NULL)
                return 0;
        else {
                node* new_node = (node*)malloc(sizeof(node));
                if (new_node == NULL)
                        return 0;
                else {
                        new_node->pid = new_p;
                        new_node->next = NULL;
                }
                new_node->next = queue->front;
		queue->front = new_node;
                (queue->total_count)++;
        return 1;
        }
}


pid_t dequeue(hnode* queue) {
	if (queue == NULL) {
		return 0;
	}
	else {
		node* temp_node = (node*)malloc(sizeof(node));
		if (temp_node == NULL)
			return 0;
		if (queue->total_count == 0)
			return 0;
		pid_t ret_p = queue->front->pid;
		temp_node = queue->front;
		if (queue->total_count == 1)
			queue->front = queue->rear = NULL;
		else
			queue->front = temp_node->next;
		(queue->total_count)--;
		free(temp_node);
		return ret_p;
	}
}
void destroyQueue(hnode* queue) {
	node* temp_node;
	if (queue != NULL) {
		while (queue->total_count > 0) {
			temp_node = queue->front;
			queue->front = temp_node->next;
			(queue->total_count)--;
			free(temp_node);
		}
		free(queue);
	}
}
hnode* createQueue()
{
	hnode* queue;
	queue = (hnode*)malloc(sizeof(hnode));
	if (queue)
	{
		queue->front = NULL;
		queue->rear = NULL;
		queue->total_count = 0;
	}
	return queue;
}

void schedule() {
	//printf("schedule\n");
	if (run_q->total_count > 0) {
		enqueue(run_q, dequeue(run_q));
		//printf("round-robin is executed\n");
	}
}
void waitqueue() {
	if (wait_q->total_count == 0)
	{
		//printf("wait queue empty!!\n");
		return;
	}
	else {
		int i, p;
		for (i = 0; i < wait_q->total_count; i++)
		{
			io_time[i]--;
			//printf("%d : io_burst is decreased\n", wait_q->total_count);
			if (io_time[i] <= 0)
			{
				run_enqueue(run_q, dequeue(wait_q));
				for (p = 0; p < wait_q->total_count; p++)
					io_time[p] = io_time[p + 1];
				//printf("first wait_q go to run_q\n");
			}
		}

	}
}

void time_tick(int signo)
{
	if (global_time_tick > 1800) {
		pid_t temp;
		while (run_q->total_count) {
			temp = dequeue(run_q);
			kill(temp, SIGKILL);
		}
		while (wait_q->total_count) {
			temp = dequeue(run_q);
			kill(temp, SIGKILL);
		}
		kill(getpid(), SIGKILL);
		return;
	}

	int a;
	printf("run_q : ");
	if (run_q->total_count>0)
	{
		node* temp = run_q->front;
		for (a = 1; a <=  run_q->total_count; temp=temp->next, a++)
			printf("<%d>", temp->pid);
	}
	printf("\n");
	printf("wait_q : ");
	if (wait_q->total_count>0)
	{
		node* temp = wait_q->front;
		for (a = 1; a <=  wait_q->total_count; temp = temp->next, a++)
			printf("<%d>", temp->pid);
	}
	printf("\n");


	waitqueue();
	if (time_quantum>0) {
		time_quantum--;
		kill(run_q->front->pid, SIGUSR1);
		printf("send signal to %d, remain time quantum: %d\n", run_q->front->pid, time_quantum);
	}
	else {
		schedule();
		time_quantum = 10;
		printf("time_quantum is reseted as 10\n");
	}
	global_time_tick++;
}
void use_cpu(int signo) {
	//printf("use cpu \n");
	(*cpu_bust)--;
	printf("my pid: %d, cpu_bust: %d\n", getpid(), *cpu_bust);

	if (*cpu_bust <= 0) {
		//printf("finish cpu bust\n");
		msg cmsg;
		if ((*msg_id = msgget((key_t)2194, IPC_CREAT | 0666)) < 0);
			//printf("msgget error\n");
		cmsg.mtype = 1;
		cmsg.pid = getpid();
		cmsg.io_time = (*io_bust);
		srand(time(NULL) + getpid());
		*cpu_bust = rand() % 30;
		msgsnd(*msg_id, &cmsg, (sizeof(msg) - sizeof(long)), IPC_NOWAIT);
	}
	return;
}
/*
void use_io() {
printf("use_io\n");
(*io_bust)--;
printf("my pid: %d, io_bust: %d\n", getpid(), *io_bust);
if ((*io_bust) <= 0) {
msg smsg;
smsg.mtype = 0;
smsg.pid = getpid();
msgsnd(*msg_id, &smsg, (sizeof(msg)-sizeof(long)), IPC_NOWAIT);
srand(time(NULL) + getpid());
*cpu_bust = rand() % 30;
*io_bust = rand() % 10;
}
}
*/
void child_process() {
	cpu_bust = (int*)malloc(sizeof(int));
	io_bust = (int*)malloc(sizeof(int));
	srand(time(NULL) + getpid());
	*cpu_bust = rand() % 30;
	*io_bust = rand() % 10;
	msg_id = (key_t*)malloc(sizeof(key_t));
	printf("child process id : %d, cpu_bust : %d, io_bust : %d\n", getpid(), *cpu_bust, *io_bust);

	struct sigaction cpu_handler;
	//struct sigaction io_handler;
	memset(&cpu_handler, 0, sizeof(cpu_handler));
	cpu_handler.sa_handler = &use_cpu;
	sigaction(SIGUSR1, &cpu_handler, NULL);

	//io_handler.sa_handler = &use_io;
	//sigaction(SIGUSR2, &io_handler, NULL);

	while (1) {
		sched_yield();
	}
}

int main(int argc, char* argv) {
	output_file = fopen("./schedule.txt", "w+");
	run_q = createQueue();
	wait_q = createQueue();

	pid_t pids[TOTALFORK];
	int runProcess = 0;
	pid_t new_child;
	struct sigaction intset;
	struct itimerval itimer;
	time_quantum = 10;

	key_id = msgget((key_t)2194, IPC_CREAT | 0666);
	if (key_id == -1) {
		perror("msgget error");
		exit(0);
	}
	else;
		//printf("Message Queue key is %d\n", key_id);

	while (runProcess < TOTALFORK) {
		pids[runProcess] = fork();
		if (pids[runProcess] < 0) {
			return -1;
		}
		else if (pids[runProcess] == 0) {
			child_process();
			exit(0);
		}
		else {
			printf("parent %ld, child %ld\n", (long)getpid(), (long)pids[runProcess]);
			new_child = pids[runProcess];
			enqueue(run_q, new_child);
		}
		runProcess++;
	}
	intset.sa_handler = &time_tick;
	sigaction(SIGALRM, &intset, NULL);

	global_time_tick = 0;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 100000;
	itimer.it_value.tv_sec = 1;
	itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itimer, NULL);
	while (1) {
		int ret;
		msg cmsg;

		if ((ret = msgrcv(key_id, &cmsg, (sizeof(msg) - sizeof(long)), 1, 0)) < 0)
		{
			continue;
		}
		else
		{
			//printf("message is recieved\n");
			enqueue(wait_q, dequeue(run_q));
			io_time[(wait_q->total_count) - 1] = cmsg.io_time;
			time_quantum = 10;
		}
	}
	return 0;
}


