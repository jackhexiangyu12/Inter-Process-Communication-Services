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

    

    mq_snd_open = mq_open("/mymq4", O_WRONLY);

    // make shared memory here
    int segment_id = shmget(IPC_PRIVATE, 32, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    char *sh_mem = (char *) shmat(segment_id, NULL, 0);

    /* printf("virtual address: %d\n", (int) sh_mem); */

    //char * text = "hello there\n";
    memmove(sh_mem, buffer, file_len );



    // right now I know the memory size is 32


    sprintf(buf, "%d", segment_id);
    len = strlen(buf);
    printf("%s\n", buf);
    mq_ret = mq_send(mq_snd_open, buf, len+1, 0);

    if (mq_ret == -1){
        printf(" messeage que is not working\n");

    }else{
        printf("Message q is working\n");
    }

}
