#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>

void main(){
    mqd_t mq_create, mq_snd_open;
    int mq_ret, len;
    //char buf[8192];

    mq_snd_open = mq_open("/mymq4", O_WRONLY);
    /* printf("please enter the message: "); */
    //scanf("%[^\n", buf);
    char *buf = "my name is queue\n";
    len = strlen(buf);
    mq_ret = mq_send(mq_snd_open, buf, len+1, 0);
    if (mq_ret == -1){
        printf(" messeage que is not working\n");

    }else{
        printf("Message q is working");
    }

}
