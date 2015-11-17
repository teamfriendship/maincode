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
#define QUANTUM 5

enum state{
	WAIT,
	READY
};

typedef struct Process{
	pid_t pid;
	int cpu_time;
	int io_time;
	int state;
} Proc;
	
Proc* createProc(pid_t newPid){
	Proc* p = (Proc*)malloc(sizeof(Proc));
	p->pid = newPid;
	p->cpu_time = rand()%10+6;
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

	int size;
} Queue;

void enqueue(Queue* q, Proc* newItem);
Proc* dequeue(Queue* q);

Queue* createQueue(){
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->size = 0;
	q->head = NULL;
	q->tail = NULL;
	return q;
}

void enqueue(Queue* q, Proc* newItem){
	
	Node* n = (Node*)malloc(sizeof(Node));

	n->item.pid = newItem->pid;
	n->item.cpu_time = newItem->cpu_time;
	n->item.io_time = newItem->io_time;
	n->item.state = newItem->state;
	n->next = NULL;

	if(q->size == 0){
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

void setBurst(Proc* proc){
	proc->io_time = srand((unsigned)time(NULL))%3;
	proc->cpu_time = srand((unsigned)time(NULL))%10 + 6;
}

typedef struct msgbuf{
	long msg_type;
	pid_t msg_pid;
	int msg_io_time;
} msg_buf;

int tick;

void timer_handler(int signum){
	printf("On time %d\n", ++tick);
	int i;
/*
	if(waitQ->size != 0){
		for(i = 0; i < waitQ.size; i++){
			waitQ->head->item.io_time--;
			
			if(waitQ->head->item.io_time == 0){
				setBurst(waitQ->head->item.io_time;
				enqueue(runQ, dequeue(waitQ));
			}
		}
	}

	if(kill(runQ->head->item.pid, SIGUSR1)==0){
	
	}
*/	
}

void timer_set(){
        //setitimer var
        struct sigaction sa;
        struct itimerval timer;

        //Install timer_handler as the signal handler for SIGALRM
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &timer_handler;
        sigaction(SIGALRM, &sa, NULL);

        //Configure the timer to expire after 250msec
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 10000;

        //and every 250msec after that
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 10000;

        //Start a virtual timer. It counts down whenever this process is executing.
        setitimer(ITIMER_REAL, &timer, NULL);
	
}
void cpu_handle(int signum){
	
}
void io_handle(int signum){
	
}

int main(){
	//msg queue var
	key_t key_id;
	int i = 0;
	msg_buf msg, rcvbuf;

	key_id = msgget((key_t)1234, IPC_CREAT|0644);

        if(key_id == -1){
                perror("msgget");
                exit(0);
        }

	//procss var
	int runningProc = 0;
	pid_t pid, ppid;
	Proc* currentProc;
	
	int tick = 0;

	Queue* runQ = createQueue();
	Queue* waitQ = createQueue();
	Queue* retireQ = createQueue();	

	//timer_set();

	Node* temp = (Node*)malloc(sizeof(Node));
	temp = runQ->head;

	signal(SIGUSR1, io_handle);
	signal(SIGUSR2, cpu_handle);

	while(runningProc < TOTALFORK){
		pid = fork();
		
		if(pid < 0){
			return -1;
		}
		else if(pid == 0){
			while(1){
				kill(getpid(), SIGUSR1);
				kill(getpid(), SIGUSR2);
			}
			
			exit(0)
		}
		else{	
			currentProc = createProc(pid);
			enqueue(runQ, currentProc);
			printf("Parent %ld, child %ld\n", (long)getpid(), (long)pid);
			
			while(1){
				kill(pid, SIGUSR2);
					
			}	
		}
		runningProc++;
	}

	timer_set();

	printQueue(runQ);
	return 0;
}
