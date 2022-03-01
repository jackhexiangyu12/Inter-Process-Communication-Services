#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/shm.h>


#include <time.h>

#define QUEUE_ID_LEN 7

#define SHARED_MEM_LEN 32
FILE *file;
void main(int argc, char *argv[]){
    mqd_t mq_create, mq_snd_open;
    int mq_ret, len;
    char buf[8192];

    //Create and open file for send
    unsigned char *buffer;
    unsigned long file_len;

    //open the file
    file = fopen("client_input.txt", "r+");
    if (file == NULL) {
        fprintf(stderr, "Unable to open file %s\n", argv[1]);
        return;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    file_len=ftell(file);
    fseek(file, 0, SEEK_SET);

    //Allocate memory
    buffer=(char *)malloc(file_len);
    if (!buffer)
    {
        fprintf(stderr, "Memory error!");
        fclose(file);
        return ;
    }

    fread(buffer,file_len,sizeof(unsigned char),file);
    fclose(file);



    mq_snd_open = mq_open("/mymq5", O_WRONLY);

    // make shared memory here
    int segment_id = shmget(IPC_PRIVATE, SHARED_MEM_LEN, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    char *sh_mem = (char *) shmat(segment_id, NULL, 0);

    /* printf("virtual address: %d\n", (int) sh_mem); */

    //char * text = "hello there\n";
    memmove(sh_mem, buffer, file_len );


    // random number generator init
    srand(time(0));

    int upper = 9999999;
    int lower = 1000000;
    int randomId = (rand() % (upper - lower + 1)) + lower;


    // create a new message queue for recieving a return message
    struct mq_attr attr;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 50;
    char id[8]; // 7 long

    sprintf(id, "%d", randomId);

    char idPath[9];
    sprintf(idPath, "/%d", randomId);

    mqd_t mq_return = mq_open(idPath, O_CREAT | O_RDWR, 0777, &attr);
    // need to error check to see that this queue is new


    printf("segment id: %d\n", segment_id);
    sprintf(buf, "%d%d", randomId, segment_id); // id, then stringified segment id
    len = strlen(buf);
    printf("the q id: %d\n", randomId);
    printf("the whole message: %s\n", buf);


    mq_ret = mq_send(mq_snd_open, buf, len+1, 0);
    if (mq_ret == -1){
      printf(" messeage que is not working\n");

    }else{
      printf("Message q is working\n");
    }

    char return_message_buffer[64];

    // now do sync call to wait to receive a message back
    mq_receive(mq_return, return_message_buffer, sizeof(return_message_buffer), NULL);
    printf("message queue has: %s\n", return_message_buffer);

    // now destroy the message queue
    mq_close(mq_return);
    mq_unlink("/abcdefg");

}
