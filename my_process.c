#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>

void main(){
    mqd_t mq_create;
    int mq_ret, i;
    struct mq_attr attr;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 50;
    mq_create = mq_open("/mymq4", O_CREAT | O_RDWR, 0777, &attr);
    if (mq_create == 0){
        printf(" messeage que is working\n");
    
    }else{
        printf("Message queue not working\n");
    }

}