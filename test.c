#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <signal.h> 
#include <errno.h>  
#include <string.h>

#define MY_MSGQ_KEY 8101

void time_tick(int);  
void do_io();
void schedule();
int *msgq_id;
int global_tick = 0;
int CPU_TIME = 0;
enum proc_state{ 
	READY, 
 	WAIT, 
}; 
 
 
typedef struct{ 
 	int pid; 
 	enum proc_state state; 
	int remaining_cpu_time; 
	int remaining_io_time;
	int turn;
}pcb; 

typedef struct{ 
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
 
int run_enqueue (QUEUE *queue, pcb* item)
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

void time_tick(int signo)  
{ 
 	if(global_tick > 600000){
	pcb *t;

	while(!emptyQueue(run_q)){
		t = dequeue(run_q);
		kill(t->pid, SIGKILL);
	}
	while(!emptyQueue(retired_q)){
		t = dequeue(retired_q);
		kill(t->pid, SIGKILL);
	}
	kill(getpid(), SIGKILL);
	}
	else
	{		
		if(!emptyQueue(retired_q))
        	{
                	do_io();
        	}
	        else
		{
			printf("IO retired_q is empty\n");
		}
		schedule();
	} 
} 

void do_cpu(int cpu_quantum)
{
	int ret;
	message msg;

	if((run_q->front->data->remaining_cpu_time)>cpu_quantum)
	{
		(run_q->front->data->turn)++;
		CPU_TIME += cpu_quantum;
		(run_q->front->data->remaining_cpu_time)-=cpu_quantum;
		if(queueCount(run_q)>1){
		enqueue(run_q, dequeue(run_q));}
	}
	else
	{
		(run_q->front->data->turn)--;
		CPU_TIME += (run_q->front->data->remaining_cpu_time);
		(run_q->front->data->remaining_cpu_time)+=((run_q->front->data->turn)*cpu_quantum);
		run_q->front->data->turn=1;
		(run_q->front->data->state)=WAIT;
		msg.type = (run_q->front->data->pid);
		msg.time_quantum = (run_q->front->data->remaining_io_time);
		ret = msgsnd(*msgq_id,&msg,(sizeof(msg)-sizeof(long)),IPC_NOWAIT);
		if(ret == -1)
		{
			perror("msgsnd() Fail");
		}
		enqueue(retired_q, dequeue(run_q));
	}
}

void do_io()
{
	int ret;
	int q_count=0;
	QUEUE_NODE *io_point;
	QUEUE_NODE *B_point;
	io_point=(retired_q->front);
	B_point=(retired_q->front);
	message msg;
		
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

	for(q_count=0;q_count<queueCount(retired_q);q_count++)
	{

		if((io_point->data->remaining_io_time)>0)
		{
			(io_point->data->remaining_io_time)--;
			
			if(queueCount(retired_q)>1)
				io_point = io_point->next;
		}
	}

	 for(q_count=0;q_count<queueCount(retired_q);q_count++)
        {
		io_point = retired_q->front;
		if(io_point->data->remaining_io_time>0)
		{
			enqueue(retired_q, dequeue(retired_q));
		}
		else
		{
                	if((ret = msgrcv(*msgq_id, &msg, (sizeof(msg)-sizeof(long)),(io_point->data->pid),IPC_NOWAIT))<0)
                       	 {
                       	         perror("msgrcv error\n");
                       	         return;
                       	 }
                       	 (io_point->data->state)=READY;
                       	 (io_point->data->remaining_io_time)=msg.time_quantum;
       	                 run_enqueue(run_q, dequeue(retired_q));
       	                 q_count--;
		}
        }



}

void schedule()
{
	int cpu_quantum = 10;

	if(!emptyQueue(run_q))
	{
		do_cpu(cpu_quantum);
	}
	else
	{
		printf("CPU run_q is empty\n");
	}
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
	QUEUE_NODE *point;
 	srand((unsigned)time(NULL)); 

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
		pid = fork();
 		if (pid == 0){ 
			//printf("child[%d]: getpid() = %d\n",i, getpid());
			return 0;
 		} else if (pid < 0) { 
 			printf("fork failed \n"); 
 			return 0; 
 		} else {
			printf("parent: pid[%d] = %d\n", i, pid);
                        proc = (pcb*)malloc(sizeof(pcb));
                        memset(proc, 0, sizeof(pcb));
                        proc->pid = pid;
                        proc->state = READY;
                        proc->remaining_cpu_time = rand()%100;
                        proc->remaining_io_time = rand()%5;
			proc->turn = 1;
                        printf("cpu time get : %d\n" , proc->remaining_cpu_time);
                        printf("io time get : %d\n" , proc->remaining_io_time);
                        enqueue(run_q, proc);
 		}
	}

 	memset(&new_sighandler, 0, sizeof(new_sighandler)); 
 	new_sighandler.sa_handler = &time_tick; 
 	//sigaction(SIGALRM, &new_sighandler, &old_sighandler); 

 	itimer.it_interval.tv_sec = 0; 
 	itimer.it_interval.tv_usec = 500000;
 	itimer.it_value.tv_sec = 0; 
 	itimer.it_value.tv_usec = 500000; 
 	global_tick = 0; 
 	setitimer(ITIMER_REAL, &itimer, &old_timer);
	while (1) {
	//setitimer(ITIMER_REAL, &itimer, &old_timer);
	sigaction(SIGALRM, &new_sighandler, &old_sighandler);
		global_tick = CPU_TIME;
		printf("run_queue : ");
		point=(run_q->front);
		for(i=0;i<queueCount(run_q);i++)
		{
			printf("<%d:%d,%d>",(point->data->pid)%10,point->data->remaining_cpu_time,point->data->remaining_io_time);
			point=(point->next);
		}
		printf("\nretired_queue : ");
		point=(retired_q->front);
		for(i=0;i<queueCount(retired_q);i++)
                {
                         printf("<%d:%d,%d>",(point->data->pid)%10,point->data->remaining_cpu_time,point->data->remaining_io_time);
                        point=(point->next);
                }
		printf("\nTime : %d\n", global_tick);
		//sleep(1);
	}


} 

