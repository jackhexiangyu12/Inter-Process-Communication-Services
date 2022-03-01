#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/shm.h>

void main(){
    mqd_t mq_create, mq_rec_open;
    int mq_ret, len;
    char buf[8192];

    mq_rec_open = mq_open("/mymq4", O_RDONLY);
    mq_ret = mq_receive(mq_rec_open, buf, sizeof(buf), NULL);

    // buf now holds the string with a number

    int segment_id = atoi(buf);

    printf("segment id: %d\n", segment_id);

    char *sh_mem = (char *) shmat(segment_id, NULL, 0);

    /* printf("virtual address: %d\n", (int) sh_mem); */


    printf("message: %s\n", sh_mem);

    shmdt(sh_mem);
    shmctl(segment_id, IPC_RMID, 0);

    if (mq_ret == -1){
        printf(" messeage que is not working\n");

    }else{
        printf("Message q is working\n");
    }

}
