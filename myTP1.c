#include<stdio.h>
#include<stdlib.h>

#define TOTALFORK 10

#define READY 0
#define WAIT 1

typedef struct Process{
	pid_t pid;
	int cpu_time;
	int io_time;
	int state;
} Proc;

typedef struct Node{
	Proc item;
	struct Node* next;
} Node;

typedef struct Queue{
	Node* head;
	Node* tail;

	void (*enqueue) (struct Queue*, Proc*);
	//void (*pop) (struct Queue*);
	//void (*peek) (struct Queue*);
 	//void (*display) (struct Queue*);
	
	int size;
} Queue;

Proc* initProc(pid_t newPid){
	Proc* newProc = (Proc*)malloc(sizeof(Proc));
	newProc->pid = newPid;
	newProc->cpu_time = rand()%50;
	newProc->io_time = rand()%3;
	newProc->state = READY;
	return newProc;
}


void enqueue(Queue* queue, Proc* newItem);

Queue createQueue(){
	Queue queue;
	queue.size = 0;
	queue.head = NULL;
	queue.tail = NULL;
	queue.enqueue = &enqueue;
	//queue.pop = &pop;
	//queue.peek = &peek;
	//queue.display = &display;
	return queue;
}

void enqueue(Queue* queue, Proc* newItem){
	Node* n = (Node*)malloc(sizeof(Node));
	n->item.pid = newItem->pid;
	n->item.cpu_time = newItem->cpu_time;
	n->item.io_time = newItem->io_time;
	n->next = NULL;

	if(queue->head == NULL){
		queue->head = n;
	}
	else{
		queue->tail->next = n;
	}
	queue->tail = n;
	queue->size++;
}

void printQueue(Queue* queue)
{
	int j;
	Node* q = (Node*)malloc(sizeof(Node));
	q = queue->head;

	for(j=0; j < queue->size; j++, q = q->next)  printf("%d ", q->item.pid);
        printf("\n");
}

int main(int argc, char* argv){

	//pid_t pids[TOTALFORK];
	int runProcess = 0;
	pid_t pid;

	Queue runQ = createQueue();
	Queue waitQ = createQueue();
	Queue retireQ = createQueue();

	while(runProcess < TOTALFORK){
		//pids[runProcess] = fork();
		pid = fork();
		Proc* process = (Proc*)malloc(sizeof(Proc));
		process = initProc(pid);
		runQ.enqueue(&runQ, process);
		printQueue(&runQ);
		printf("\n");


		if(/*pids[runProcess]*/pid < 0){
			return -1;
		}
		else if(/*pids[runProcess]*/pid == 0){
			int i =0;
			for(i=0; i<5; i++){
				printf("child process %ld : number(%d) \n", (long)getpid(), i);
			}
			exit(0);
		}else{
			printf("parent %ld, child %ld\n", (long)getpid(),
				/*(long)pids[runProcess]*/(long)pid);
		}
		runProcess++;
	}
	return 0;
}
