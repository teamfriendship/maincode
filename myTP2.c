#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

#define TOTALFORK 10
#define READY 0
#define WAIT 1
#define QUANTUM 5

typedef struct Process{
	pid_t pid;
	int cpu_time;
	int io_time;
	int state;
} Proc;
	
Proc* createProc(pid_t newPid){
	Proc* p = (Proc*)malloc(sizeof(Proc));
	p->pid = newPid;
	p->cpu_time = rand()%10;
	p->io_time = rand()%3;
	p-> state = READY;
	return p;
}

typedef struct Node{
	Proc item;
	struct Node* next;
} Node;

typedef struct Queue{
	Node* head;
	Node* tail;

	void (*enqueue) (struct Queue*, Proc*);
	Proc* (*dequeue) (struct Queue*);

	int size;
} Queue;

void enqueue(Queue* q, Proc* newItem);
Proc* dequeue(Queue* q);

Queue* createQueue(){
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->size = 0;
	q->head = NULL;
	q->tail = NULL;
	q->enqueue = &enqueue;
	q->dequeue = &dequeue;
	return q;
}

void enqueue(Queue* q, Proc* newItem){
	Node* n = (Node*)malloc(sizeof(Node));
	n->item.pid = newItem->pid;
	n->item.cpu_time = newItem->cpu_time;
	n->item.io_time = newItem->io_time;
	n->next = NULL;

	if(q->head == NULL){
		q->head = n;
	}
	else{
		q->tail->next = n;
	}

	q->tail = n;	
	q->size++;
}

Proc* dequeue(Queue* q){
	Node* n = q->head;
	Proc* oldItem = &(n->item);
	q->head = n->next;
	q->size--;
	free(n);
	return oldItem;	
}

void printQueue(Queue* q){
	int i;
	Node* n = (Node*)malloc(sizeof(Node));
	n = q->head;
	
	printf("< ");
	for(i = 0; i < q->size; i++, n = n->next) printf("%d ", n->item.pid);
	printf(">");
	printf("\n");
}

typedef struct msgbuf{
	long msg_type;
	char msg_text;
} msg_buf;

void timer_handler(int signum){
	static int count = 0;
	printf("timer expired %d timers\n", ++count);
}

void sig_handler(int signo){
	psignal(signo, "Received Signal: ");
	switch(signo){
		case
	}
}

int main(){
	//msg queue var
	key_t key_id;
	int i;
	msg_buf mybuf, rcvbuf;

	//procss var
	int runningProc = 0;
	pid_t pid;
	Proc* currentProc;
	
	int tick = 0;

	Queue* runQ = createQueue();
	Queue* waitQ = createQueue();
	Queue* retireQ = createQueue();	
	
	//setitimer var
	struct sigaction sa;
	struct itimerval timer;
	
	//Install timer_handler as the signal handler for SIGVTALRM
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sig_handler;
	sigaction(SIGVTALRM, &sa, NULL);

	//Configure the timer to expire after 250msec
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 250000;

	//and every 250msec after that
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 250000;
	
	//Start a virtual timer. It counts down whenever this process is executing.
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	key_id = msgget((key_t)1234, IPC_CREAT|0666);

	if(key_id == -1){
		perror("msgget");
		exit(0);
	}	

	printf("\n");

	Node* temp = (Node*)malloc(sizeof(Node));
	temp = runQ->head;
	while(1){
		pid = fork();
		currentProc = createProc(pid);
		
		if(pid < 0) return -1;
		else if(pid == 0){
			for(i = 0; i < QUANTUM; i++, temp->item.cpu_time--, temp = temp->next){		
                                if(temp->item.cpu_time == 0){
                                        kill(temp->item.pid, SIGKILL);
                                }

				printf("child process %ld\ntime quantum (%d), remaining cpu time (%d)\n", (long)temp->item.pid, i, temp->item.cpu_time);

				printf("run Queue : ");
				printQueue(runQ);
				printf("wait Queue : ");
				printQueue(waitQ);
				printf("retire Queue : ");
				printQueue(retireQ);	
				printf("\n");	
			}

			
		        sa.sa_flags = 0;
   			sa.sa_handler = sig_handler;

		        if(sigaction(SIGINT, &sa, (struct sigaction*)NULL) < 0){
		                perror("SIGACTION");
      			        exit(1);
        		
			}
			
			if(temp->item.cpu_time != 0){
				enqueue(retireQ, dequeue(runQ));
			}
			
			exit(0);
		}
		else{
			printf("parent process %ld, child process %ld\n", (long)getpid(), (long)currentProc->pid);
		}
		enqueue(runQ, currentProc);
		printQueue(runQ);
		printf("\n");
		runningProc++;
	}
	return 0;
}
