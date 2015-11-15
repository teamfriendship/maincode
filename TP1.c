#include<stdio.h>
#include<stdlib.h>

#define TOTALFORK 3

typedef struct p_strcut{
	pid_t cpid;
	int cputime;
	int iotime;
};

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
