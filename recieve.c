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

#define QUEUE_ID_LEN 7

void main(){
    mqd_t mq_create, mq_rec_open;
    int mq_ret, len;
    char buf[8192];

    mq_rec_open = mq_open("/mymq5", O_RDONLY);
    mq_ret = mq_receive(mq_rec_open, buf, sizeof(buf), NULL);

    // buf now holds the string of: <7 char id><segment id number>

    char mq_id[8];

    memcpy(mq_id, buf, QUEUE_ID_LEN);
    mq_id[QUEUE_ID_LEN] = '\0';

    printf("return q id: %s\n", mq_id);

    int i = 7;
    int j = 0;
    char segment_id_str_buffer[100];
    while (buf[i] != '\0') {
      segment_id_str_buffer[j] = buf[i];

      i++;
      j++;
    }
    segment_id_str_buffer[j + 1] = '\0';

    int segment_id = atoi(segment_id_str_buffer);

    printf("segment id: %d\n", segment_id);

    char *sh_mem = (char *) shmat(segment_id, NULL, 0);

    /* printf("virtual address: %d\n", (int) sh_mem); */


    printf("message: %s\n", sh_mem);

    char messageBuff[211];
    sprintf(messageBuff, "modified message string : %s", sh_mem);
    /* sprintf(messageBuff, "modified message string : %s", "fake message"); */

    shmdt(sh_mem);
    shmctl(segment_id, IPC_RMID, 0);



    // now open the return message q, and put a message on it

    char mqPath[100];
    sprintf(mqPath, "/%s", mq_id);
    /* printf("%s\n", mqPath); */

    mqd_t return_q = mq_open(mqPath, O_WRONLY);
    int return_status = mq_send(return_q, messageBuff, strlen(messageBuff) + 1, 0);

    if (mq_ret == -1){
        printf(" messeage que is not working\n");

    }else{
        printf("Message q is working\n");
    }

}
