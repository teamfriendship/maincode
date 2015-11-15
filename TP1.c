#include<stdio.h>
#include<stdlib.h>

#define TOTALFORK 10


typedef struct p_struct{
        pid_t cpid;
        int cputime;
        int iotime;
}pcb;
typedef struct q_node{
	struct p_struct* p_data;
	struct q_node* next;
}node;
typedef struct QUEUE{
	int total_count;
	struct q_node* front;
	struct q_node* rear;
}hnode;

hnode* run_q;
hnode* retired_q;

int enqueue(hnode* queue, pcb* new_p){
	if(queue == NULL)
		return 0;
	else{
		node* new_node = (node*)malloc(sizeof(node));
		if(new_node == NULL)
			return 0;
		else{
			new_node->p_data = new_p;
			new_node->next = NULL;
		}
		if(queue->total_count == 0)
			queue->front = new_node;
		else
			queue->rear->next = new_node;
		
		(queue->total_count)++;
		queue->rear = new_node;
		return 1;
	}
}
pcb* dequeue(hnode* queue){
	if(queue == NULL){
		return 0;
	}
	else{
		node* temp_node = (node*)malloc(sizeof(node));
		if(temp_node == NULL)
			return NULL;
		if(queue->total_count == 0)
			return NULL;
		pcb* ret_p = queue->front->p_data;
		temp_node = queue->front;
		if(queue->total_count == 1)
			queue->front = queue->rear = NULL;
		else
			queue->front = temp_node->next;
		(queue->total_count)--;
		free(temp_node);
		return ret_p;						
	}
}
void destroyQueue(hnode* queue){
	node* temp_node;
	if(queue != NULL){
		while(queue->total_count > 0){
			temp_node = queue->front;
			queue->front = temp_node->next;
			(queue->total_count)--;
			free(temp_node);
		}
		free(queue);
	}
}

		
	

int main(int argc, char* argv){

	pid_t pids[TOTALFORK], pid;
	int runProcess = 0;
	int state;

	while(runProcess < TOTALFORK){
		pids[runProcess] = fork();
		if(pids[runProcess] < 0){
			return -1;
		}
		else if(pids[runProcess] == 0){
			int i =0;
			for(i=0; i<5; i++){
				printf("child process %ld : number(%d) \n", (long)getpid(), i);
			}
			exit(0);
		}else{
			printf("parent %ld, chile %ld\n", (long)getpid(),
				(long)pids[runProcess]);
		}
		runProcess++;
	}
	return 0;
}
