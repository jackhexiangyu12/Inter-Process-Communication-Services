#include <stdio.h>
#include <stdlib.h>
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

#include "snappy.h"
#include "task_queue.h"
#include "include.h"


mqd_t setup_main_q() {
  mqd_t mq_create;
  struct mq_attr attr;
  attr.mq_curmsgs = 0;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = 50;
  mq_create = mq_open(MAIN_QUEUE_PATH, O_CREAT | O_RDWR, 0777, &attr);
  return mq_create;
}

// need function where you give it a byte array, and it puts a proper shared memory message on q
void return_compressed_data(unsigned char *compressed_data, unsigned long compressed_len, char *mqId) {
  char mqPath[100];
  sprintf(mqPath, "/%s", mqId);


  // make new shared memory for sending back the compressed file
  int segment_id = shmget(IPC_PRIVATE, compressed_len, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  memmove(sh_mem, compressed_data, compressed_len);

  char return_message_buf[218];
  sprintf(return_message_buf, "%d", segment_id);


  mqd_t return_q = mq_open(mqPath, O_WRONLY);
  int return_status = mq_send(return_q, return_message_buf, strlen(return_message_buf) + 1, 0);

  if (return_status == -1) {
    printf(" messeage que is not working\n");

  } else {
    printf("Message q is working\n");
  }


  // dont need anything else?
}

int extract_segment_id(char *mqMessage) {
  // extract the segment id from the message
  int i = 7;
  int j = 0;
  char segment_id_str_buffer[100];
  while (mqMessage[i] != '\0') {
    segment_id_str_buffer[j] = mqMessage[i];

    i++;
    j++;
  }
  segment_id_str_buffer[j + 1] = '\0';

  int segment_id = atoi(segment_id_str_buffer);
  return segment_id;
}

void extract_mqID(char *mqMessage, char **mq_id) {
  // cant figure this pointer stuff out
}

void handle_request(char *mqMessage) {
  // buf now holds the string of: <7 char id><segment id number>

  // extract mq_id from the message
  char mq_id[8];
  memcpy(mq_id, mqMessage, QUEUE_ID_LEN);
  mq_id[QUEUE_ID_LEN] = '\0';

  printf("return q id: %s\n", mq_id);


  int segment_id = extract_segment_id(mqMessage);

  printf("segment id: %d\n", segment_id);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  /* printf("virtual address: %d\n", (int) sh_mem); */


  /* printf("message: %s\n", sh_mem); */

  /* sprintf(messageBuff, "modified message string : %s", "fake message"); */


  unsigned char *compressed_file;

  // now need to do the actual compression, and get a length of the compressed file
  unsigned long compressed_file_length;
  // TODO:
  struct snappy_env env;
  snappy_free_env(&env);
  
  snappy_compress(&env, sh_mem, 8192, compressed_file, &compressed_file_length);



  // kill the shared memory that was used to grab the original file
  shmdt(sh_mem);
  shmctl(segment_id, IPC_RMID, 0);




  // now open the return message q, and put a message on it

  /* printf("%s\n", mqPath); */
  return_compressed_data(compressed_file, compressed_file_length, mq_id);

}


int main() {
  /*

    set up message queue for use

    set up 30ms timer

    set up active queue and current task pointers


   */

  mqd_t setup_result = setup_main_q();
  if (setup_result == -1) {
    printf("couldnt make the q\n");
    return 1;
  }

  mqd_t mq_rec_open;
  int mq_ret;

  int dont_halt = 1;
  while (dont_halt) {
    // put current task onto the active queue

    // check message queue for task
    // if no task, check active queue and switch to it
    // if message queue has a task, need to make new uthread for that task
    char recieve_buffer[8192];

    mq_rec_open = mq_open(MAIN_QUEUE_PATH, O_RDONLY);
    mq_ret = mq_receive(mq_rec_open, recieve_buffer, sizeof(recieve_buffer), NULL);

    if (mq_ret == -1) {
      printf(" messeage que is not working\n");

    } else {
      printf("Message q is working\n");
    }

    handle_request(recieve_buffer);




    // this is just some stub stuff to make sure the makefile works
    active_q * tq = get_active_q();
    dont_halt = tq->stub;
  }
}
