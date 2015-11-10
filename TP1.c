#include<stdio.h>
#include<stdlib.h>

#define TOTALFORK 10

typedef struct Node{
	pid_t item;
	struct Node* next;
} Node;

typedef struct Queue{
	Node* head;
	Node* tail;

	void (*push) (struct Queue*, pid_t);
	//void (*pop) (struct Queue*);
	//void (*peek) (struct Queue*);
 	//void (*display) (struct Queue*);
	
	int size;
} Queue;

void push(Queue* queue, pid_t pid);

Queue createQueue(){
	Queue queue;
	queue.size = 0;
	queue.head = NULL;
	queue.tail = NULL;
	queue.push = &push;
	//queue.pop = &pop;
	//queue.peek = &peek;
	//queue.display = &display;
	return queue;
}

void push(Queue* queue, pid_t item){
	Node* n = (Node*)malloc(sizeof(Node));
	n->item = item;
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

	for(j=0; j < queue->size; j++, q = q->next)  printf("%d ", q->item);
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
		runQ.push(&runQ, pid);
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
