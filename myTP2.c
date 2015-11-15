#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>
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
	//void (*dequeue) (struct Queue*);

	int size;
} Queue;

void enqueue(Queue* q, Proc* newItem);

Queue* createQueue(){
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->size = 0;
	q->head = NULL;
	q->tail = NULL;
	q->enqueue = &enqueue;
	//queue->dequeue = &dequeue;
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

void printQueue(Queue* q){
	int i;
	Node* n = (Node*)malloc(sizeof(Node));
	n = q->head;
	
	printf("<");
	for(i = 0; i < q->size; i++, n = n->next) printf("%d ", n->item.pid);
	printf(">");
	printf("\n");
}

typedef struct msgbuf{
	long msg_type;
	char msg_text;
} msg_buf;


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
	
	key_id = msgget((key_t)1234, IPC_CREAT|0666);

	if(key_id == -1){
		perror("msgget");
		exit(0);
	}	

	printf("\n");

	while(runningProc <TOTALFORK){
		pid = fork();
		currentProc = createProc(pid);
		
		if(pid < 0) return -1;
		else if(pid == 0){
			for(i = 0; i < QUANTUM; i++, currentProc->cpu_time--){		
				printf("tick(%d) : child process %ld : time quantum (%d), remaining cpu time (%d)\n", tick, (long)getpid(), i, currentProc->cpu_time);

				if(currentProc->cpu_time == 0){
					kill(currentProc->pid, SIGKILL);
				}
				
				printf("run Queue : ");
				printQueue(runQ);
				printf("wait Queue : ");
				printQueue(waitQ);
				printf("retire Queue : ");
				printQueue(retireQ);	
				printf("\n");	

				tick++;	

			}

			exit(0);
		}
		else{
			printf("parent process %ld, child process %ld\n", (long)getpid(), (long)currentProc->pid);
		}
		enqueue(runQ, currentProc);
		runningProc++;
	}
	return 0;
}
