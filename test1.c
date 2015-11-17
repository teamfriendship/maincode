#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/time.h>

#define TOTALFORK 10
#define BUFF_SIZE 1024

typedef struct q_node{
        pid_t pid;
        struct q_node* next;
}node;
typedef struct QUEUE{
        int total_count;
        struct q_node* front;
        struct q_node* rear;
}hnode;
typedef struct msg_childtoparent{
        long mtype;
        pid_t pid;
	int io_time;
}msg;

hnode* run_q;
hnode* wait_q;
int global_time_tick;
int key_id;
FILE* output_file;
int time_quantum;
int* cpu_bust;
int* io_bust;
key_t* msg_id;

int enqueue(hnode* queue, pid_t new_p){
        if(queue == NULL)
                return 0;
        else{
                node* new_node = (node*)malloc(sizeof(node));
                if(new_node == NULL)
                        return 0;
                else{
                        new_node->pid = new_p;
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
pid_t dequeue(hnode* queue){
        if(queue == NULL){
                return 0;
        }
        else{
                node* temp_node = (node*)malloc(sizeof(node));
                if(temp_node == NULL)
                        return 0;
                if(queue->total_count == 0)
                        return 0;
                pid_t ret_p = queue->front->pid;
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
hnode* createQueue()
{
        hnode* queue;
        queue = (hnode*)malloc(sizeof(hnode));
        if(queue)
        {
                queue->front = NULL;
                queue->rear = NULL;
		queue->total_count = 0;
        }
        return queue;
}

void schedule(){
	printf("schedule\n");
	if(run_q->total_count > 0){
		kill(run_q->front->pid, SIGALRM);
	}
}
void waitqueue(){
        if(wait_q->total_count == 0)
        {
                //printf("wait queue empty!!");
                return;
        }
        else{
                int ret;
                msg rmsg;
                node* temp = wait_q->front;
                for(; temp->next != NULL; temp = temp->next){
                        kill(temp->pid, SIGUSR2);
                        if((ret = msgrcv(key_id, &rmsg, sizeof(msg),0,0))<0)
                                printf("msgrcv error");
                        else{
                                enqueue(run_q, dequeue(wait_q));
                                printf("child process finish");
                        }
                }
        }
}
void time_tick()
{
	if(global_time_tick > 1800) {
		pid_t temp;
		while(run_q->total_count){
			temp = dequeue(run_q);
			kill(temp, SIGKILL);
                }
		while(wait_q->total_count){
			temp = dequeue(run_q);
			kill(temp, SIGKILL);
                }
                kill(getpid(), SIGKILL);
                return;
	}
		
	if(time_quantum>0){
		time_quantum--;
		kill(run_q->front->pid, SIGUSR1);
		printf("send signal to %d, remain time quantum: %d\n", run_q->front->pid, time_quantum);
	}
	else{
		schedule();
		time_quantum = 10;
	}		
	waitqueue();
	global_time_tick++;
}
void use_cpu(int signo){
	if(time_quantum > 0){
		printf("use cpu \n");
		(*cpu_bust)--;
		printf("my pid: %d, cpu_bust: %d", getpid(), *cpu_bust);
	}
	else{
		printf("finish cpu bust\n");
		msg cmsg;
		if((*msg_id = msgget((key_t)2194, IPC_CREAT|0666)) < 0)
			printf("msgget error\n");

		cmsg.mtype = 0;
		cmsg.pid = getpid();
		cmsg.io_time = (*io_bust);
		msgsnd(*msg_id, &cmsg, sizeof(msg), IPC_NOWAIT);
	}
	return;
}
void use_io(int signo){
	printf("use_io\n");
	(*io_bust)--;
    if((*io_bust) <= 0){
		msg smsg;
		smsg.mtype = 0;
	    smsg.pid = getpid();
		msgsnd(*msg_id, &smsg, sizeof(msg), IPC_NOWAIT);
		srand(time(NULL)+getpid());
		*cpu_bust = rand() % 100;
		*io_bust = rand() % 100;
	}
}

void child_process(){
	cpu_bust = (int*)malloc(sizeof(int));
	io_bust = (int*)malloc(sizeof(int));
	srand(time(NULL)+getpid());
	*cpu_bust = rand() % 100;
	*io_bust = rand() % 100;
	msg_id = (key_t*)malloc(sizeof(key_t));
	printf("child process id : %d, cpu_bust : %d, io_bust : %d\n", getpid(), *cpu_bust, *io_bust);

	struct sigaction cpu_handler, io_handler; 
	cpu_handler.sa_handler = &use_cpu;
	sigaction(SIGUSR1, &cpu_handler, NULL);

	io_handler.sa_handler = &use_io;
	sigaction(SIGUSR2, &io_handler, NULL);

	while(1){

	}
}

int main(int argc, char* argv){
	output_file = fopen("./schedule.txt","w+");
	run_q = createQueue();
	wait_q = createQueue();

	pid_t pids[TOTALFORK];
	int runProcess = 0;
	pid_t new_child;
	struct sigaction intset;
	struct itimerval itimer;
	time_quantum = 10;

	key_id = msgget((key_t)2194, IPC_CREAT|0666);
	if(key_id == -1){
		perror("msgget error");
		exit(0);   
	}
	else
		printf("Message Queue key is %d\n", key_id);

	while(runProcess < TOTALFORK){
                pids[runProcess] = fork();
                if(pids[runProcess] < 0){
                        return -1;
                }
                else if(pids[runProcess] == 0){
                        child_process();
                        exit(0);
                }else{
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
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = 100000;
	setitimer(ITIMER_REAL, &itimer, NULL);
	while(1){

	}
        return 0;
}
