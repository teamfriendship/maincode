#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
 
 
#define MY_MSGQ_KEY	2627  
 
#include <signal.h> 
#include <errno.h>  

 
 
void time_tick(int);  
 
int global_tick = 0; 
 
enum proc_state{ 
	READY, 
 	WAIT, 
}; 
 
 
typedef struct{ 
 	int pid; 
 	enum proc_state state; 
	int remaining_cpu_time; 
	int remaining_io_time; 
}pcb; 
  
 
typedef struct{ 
 	int mtype;  	
	int pid; 
 	int time_quantum; 
}message; 
 
 
int schedule(); 

void do_child(); 
 
void child_process(int signo); 

int * msgq_id; 
 
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
 
 
int enqueue (QUEUE *queue, pcb* item) 
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
 

pcb* dequeue (QUEUE *queue) 
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
 	free (deleteLoc); 
 	 
 	return item; 
} 
 
 
int emptyQueue (QUEUE *queue) 
{ 
	return (queue->count == 0); 
} 
 
 
int fullQueue (QUEUE *queue) 
{ 
	QUEUE_NODE *temp; 
 
 
 	if ((temp = (QUEUE_NODE*)malloc(sizeof(QUEUE_NODE)))) 
 	{ 
 		free(temp); 
 		return 0; 
 	} 
 	return 1; 
} 
 
 
int queueCount (QUEUE *queue) 
{ 
	return queue->count; 
} 
 

QUEUE *destroyQueue (QUEUE *queue) 
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
QUEUE* retired_q; 
 
pcb* current = NULL; 
 

void time_tick(int signo)  
{ 
 	int run_pid; 
 	message msg; 
 
 	if (global_tick > 600000) { 
 		pcb* t; 
 		 
 		  
 		while(!emptyQueue(run_q)){		 
 			t = dequeue(run_q); 
 			kill(t->pid,SIGKILL); 
 		} 
 		while(!emptyQueue(retired_q)){ 
 			t = dequeue(retired_q); 
 			if(t !=NULL) kill(t->pid,SIGKILL); 
 		} 
 		kill(getpid(), SIGKILL);
 		return; 
 	} 
	if((global_tick%10) == 0) {							 
 			run_pid = schedule(); 
 			if(run_pid != -1) 
 			{ 		 
 		    	msg.mtype = 0; 
 			    msg.pid = run_pid; 
 				msg.time_quantum = 10; 
 			    msgsnd(*msgq_id, &msg, sizeof(msg), 0); 
 			} 
 	} 
 
 
 	global_tick++; 
 	return ; 
} 
 
 
 
 
int main (int argc, char *argv[]) 
{ 
 	run_q = createQueue(); 
 	retired_q = createQueue(); 
 
 	int i = 0; 
 	int pid = -1; 
 	struct sigaction old_sighandler, new_sighandler; 
 	struct itimerval itimer, old_timer; 
 	int ret; 
 	message msg; 
 	pcb* proc; 
 	srand(10); 

 	msgq_id = (int *)malloc(sizeof(int)); 
        ret = msgget(MY_MSGQ_KEY, IPC_CREAT | 0644); 
	if (ret == -1) { 
        	  printf("msgq creation error ! (p) "); 
          	  printf("errno: %d\n", errno); 
          	  *msgq_id = -1; 
        } 
        else { 
          	*msgq_id = ret; 
          	printf("msgq (key:0x%x, id:0x%x) created.\n", MY_MSGQ_KEY, ret); 
        } 
 
	for (i = 0 ; i < 10 ; i++) { 
 		if ((pid = fork()) > 0){ 
			printf("parent: pid[%d] = %d\n", i, pid); 
			proc = (pcb*)malloc(sizeof(pcb)); 
			memset(proc, 0, sizeof(pcb));  
 			proc->pid = pid; 
			proc->state = READY; 
			proc->remaining_cpu_time = rand()%100; 
 			proc->remaining_io_time = rand()%100; 
 			printf("cpu time get : %d\n" , proc->remaining_cpu_time); 
 			enqueue(run_q, proc);  
 
 
 		} else if (pid < 0) { 
 			printf("fork failed \n"); 
 			return 0; 
 		} else if (pid == 0) { 
 			printf("child: getpid() = %d\n", getpid()); 
 			do_child();  
 			return 0; 
 		} 
 	} 
 

 	memset(&new_sighandler, 0, sizeof(new_sighandler)); 
 	new_sighandler.sa_handler = &time_tick; 
 	sigaction(SIGALRM, &new_sighandler, &old_sighandler); 
 	 
 
 	itimer.it_interval.tv_sec = 0; 
 	itimer.it_interval.tv_usec = 100000;
 	itimer.it_value.tv_sec = 0; 
 	itimer.it_value.tv_usec = 100000; 
 	global_tick = 0; 
 	setitimer(ITIMER_REAL, &itimer, &old_timer);
 
 
	while (1) {
 	return 0; 
 	}
} 


int remaining_cpu_burst=10; 
int remaining_io_burst = 1; 
 

void do_io() { 
 

 	message msg; 
	int ret; 
 	key_t key; 
 

 	if (*msgq_id == -1) { 
 		ret = msgget(MY_MSGQ_KEY, IPC_CREAT | 0644); 
		if (ret == -1) { 
 			printf("msgq creation error !  (c) "); 
 			printf("errno: %d\n", errno); 
 		} 
 		*msgq_id = ret; 
 	} 
 

 	msg.mtype = 0; 
 	msg.pid = getpid(); 
 	msg.time_quantum = 0; 
 	msgsnd(*msgq_id, &msg, sizeof(msg), 0); 
} 
 

void child_process(int signo) 
{ 
 

	remaining_cpu_burst--;
 

 	if (remaining_cpu_burst < 0){ 
 		return; 
 		} 
 
 	return; 
} 
 
void do_child(){ 
 	int ret; 
 	struct sigaction old_sighandler, new_sighandler; 
 	message msg; 
 
 	if((ret = msgrcv(*msgq_id, &msg, sizeof(msg), 0, 0))<0) 
 	{ 
 		perror("msgrcv:"); 
 		return ; 
 	} 

 	if(msg.pid==getpid()) 
	{ 
 		memset (&new_sighandler, 0, sizeof (new_sighandler)); 
 		new_sighandler.sa_handler = &child_process; 
 		sigaction(SIGALRM, &new_sighandler, &old_sighandler);  
 

 	} 
 
} 
 

int schedule(){ 
 
 	pcb *temp,*next; 
 	remaining_cpu_burst = 10; 
 

 	if(current == NULL) 
 	{ 
 		next = dequeue(run_q); 
 		current = next;
 	}
 	else 
 	{ 
 		current->remaining_cpu_time = current->remaining_cpu_time-10; 
 		if(!emptyQueue(run_q)){ 
 			if(current->remaining_cpu_time <=0) 
 			{ 
 					next = dequeue(run_q); 
 					printf("pid : %d , remaining cpu time : %d \n", current->pid,0); 
 					kill(current->pid,SIGKILL); 
 					current= next; 
 			}
 			else 
 			{ 
 				next = dequeue(run_q); 
 				printf("pid : %d , remaining cpu time : %d\n", current->pid,current->remaining_cpu_time); 
 				enqueue(retired_q, current); 
 				current = next; 
 			}
 		}
 		else  
 		{ 
 			if(emptyQueue(retired_q)&&current==NULL) 
 			{	 
 				kill(getpid(),SIGKILL);
 			} 
  			else if(current->remaining_cpu_time<=0) 
 			{ 
 				printf("pid : %d , remaining cpu time : %d \n", current->pid,0); 
 				kill(current->pid, SIGKILL);  
 				current = NULL; 
 				if(emptyQueue(retired_q)) 
 					kill(getpid(),SIGKILL); 
 			} 
 			else
 			{ 
 				printf("pid : %d , remaining cpu time : %d \n", current->pid, current->remaining_cpu_time); 
 				enqueue(retired_q,current); 
 			} 
			while(!emptyQueue(retired_q)) 			
 			{ 
 				temp = dequeue(retired_q); 
 				enqueue(run_q, temp);	 
 			} 
 				next = dequeue(run_q); 
 				current = next;
 		}
 	}
 	if(next == NULL) return -1; 
 	return next->pid; 
}
