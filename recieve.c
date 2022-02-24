#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

void main(){
    mqd_t mq_create, mq_rec_open;
    int mq_ret, len;
    char buf[8192];

    mq_rec_open = mq_open("/mymq2", O_RDONLY);
    mq_ret = mq_receive(mq_rec_open, buf, sizeof(buf), NULL);

    printf("message: %s\n",buf);
    
    if (mq_ret == 0){
        printf(" messeage que is working\n");
    
    }else{
        printf("Message q not working\n");
    }

}