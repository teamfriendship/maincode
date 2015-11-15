#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/types.h"
#include "sys/ipc.h"
#include "sys/msg.h"
#include <poll.h>
 
#define  BUFF_SIZE   1024
#define  KEY         35002
 
typedef struct {
    int   msgid;
    long  type;
    char  buff[BUFF_SIZE];
} t_data;
 
int main()
{
    int      msqid;
    int      index   = 0;
    int      ret   = 0;
    t_data   data;
 	int a=0;
    pid_t pid;

    printf("t_data size = %d\n", sizeof(t_data));
    printf("long size = %d\n", sizeof(long));
 
    if((msqid = msgget((key_t)KEY, IPC_CREAT|0666))== -1) {
        perror( "msgget() Fail");
        return -1;
    }

	for(a=0;a<10;a++)
	{
		if((pid = fork())<0)
		{
			perror("fork error");
			exit(1);
		}
	
		else if(pid == 0) // child
		{
		data.type  = index;
        data.msgid =  ((index++ %10)+1);

        sprintf( data.buff, "msg_type=%ld,index=%d, message queue program",
                data.type, index);

        printf( "ID=[%d]  MSG => %s\n", data.msgid, data.buff);

        /* IPC message send */
        ret = msgsnd(msqid, &data, (sizeof(t_data)-sizeof(long)), 0);
        if(ret == -1)
        {
            perror( "msgsnd() Fail");
            return -2;
        }
        /* sleep 2 seconde */
        poll(0, 0, 2000);
	
		}
		
		else //parents
		{
			
		}
	}
    return 0;
}

