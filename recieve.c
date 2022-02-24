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

    mq_rec_open = mq_open("/mymq4", O_RDONLY);
    mq_ret = mq_receive(mq_rec_open, buf, sizeof(buf), NULL);

    printf("message: %s\n",buf);

    if (mq_ret == -1){
        printf(" messeage que is not working\n");

    }else{
        printf("Message q is working\n");
    }

}
