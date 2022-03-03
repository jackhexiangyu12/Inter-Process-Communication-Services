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
#include <pthread.h>

#include "snappy.h"
#include "task_queue.h"
#include "include.h"

typedef struct thread_arg_payload {
  mqd_t main_q;
} thread_arg_t;


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
void return_compressed_data(char *compressed_data, unsigned long compressed_len, char *mqId) {
  char mqPath[100];
  sprintf(mqPath, "/%s", mqId);


  // make new shared memory for sending back the compressed file
  int segment_id = shmget(IPC_PRIVATE, compressed_len, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);


  memmove(sh_mem, compressed_data, compressed_len);
  char return_message_buf[218];
  sprintf(return_message_buf, "%d:%lu",segment_id , compressed_len);
  // compressed_len:segment_id



  mqd_t return_q = mq_open(mqPath, O_WRONLY);
  int return_status = mq_send(return_q, return_message_buf, strlen(return_message_buf) + 1, 0);

  if (return_status == -1) {
    printf(" messeage que is not working\n");

  } else {
    printf("Message q is working\n");
  }


  // dont need anything else?
}

int extract_segment_id(char *mqMessage, int colon_index) {
  // extract the segment id from the message

  char segIdCharBuf[200];

  int i = colon_index + 1;
  int j = 0;
  while (mqMessage[i] != '\0') {
    segIdCharBuf[j] = mqMessage[i];
    i++;
    j++;
  }

  segIdCharBuf[j] = '\0';
  printf("buffer pre conversion: %s\n", segIdCharBuf);
  printf("colon index: %d\n", colon_index);

  int segment_id = atoi(segIdCharBuf);
  return segment_id;
}

void extract_mqID(char *mqMessage, char **mq_id) {
  // cant figure this pointer stuff out
}

void *handle_request(void *buffer) {
  char *mqMessage = buffer;
  printf("message received: %s\n", mqMessage);
  // buf now holds the string of: <7 char id for the mq><file_len as unsigned long>:<segment id number>

  // extract mq_id from the message
  char mq_id[8];
  memcpy(mq_id, mqMessage, QUEUE_ID_LEN);
  mq_id[QUEUE_ID_LEN] = '\0';

  printf("return q id: %s\n", mq_id);

  int i = QUEUE_ID_LEN;
  int j = 0;
  char dataLenBuffer[64];
  while (mqMessage[i] != ':') {
    dataLenBuffer[j] = mqMessage[i];
    i++;
    j++;
  }
  dataLenBuffer[j] = '\0';

  char **f;
  unsigned long data_len = strtoul(dataLenBuffer, f, 10);
  printf("data length: %lu\n", data_len);


  int segment_id = extract_segment_id(mqMessage, i);

  printf("segment id: %d\n", segment_id);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  /* printf("virtual address: %d\n", (int) sh_mem); */


  /* printf("message: %s\n", sh_mem); */

  /* sprintf(messageBuff, "modified message string : %s", "fake message"); */


  char compressed_file[data_len];

  // now need to do the actual compression, and get a length of the compressed file
  unsigned long compressed_file_length = data_len;
  // TODO:
  struct snappy_env env;
  snappy_init_env(&env);

  //snappy_compress(&env, sh_mem, data_len, compressed_file, &compressed_file_length);
  memmove(compressed_file, sh_mem, data_len);



  // kill the shared memory that was used to grab the original file
  shmdt(sh_mem);
  shmctl(segment_id, IPC_RMID, 0);




  // now open the return message q, and put a message on it

  /* printf("%s\n", mqPath); */
  return_compressed_data(compressed_file, compressed_file_length, mq_id);

  pthread_exit(0);
  return NULL;
}

static void *listen_thread(void *arg) {
  thread_arg_t *thread_arg = (thread_arg_t *) arg;


  while (1) {
    // put current task onto the active queue

    // check message queue for task
    // if no task, check active queue and switch to it
    // if message queue has a task, need to make new uthread for that task
    char recieve_buffer[8192];

    // blocking call
    int mq_ret = mq_receive(thread_arg->main_q, recieve_buffer, sizeof(recieve_buffer), NULL);

    if (mq_ret == -1) {
      printf(" messeage que is not working\n");

    } else {
      printf("Message q is working\n");
    }
    // TODO: add stuff to some pthread list
    pthread_t request_handler_id;
    pthread_create(&request_handler_id, NULL, handle_request, (void *)recieve_buffer);

  }

  return NULL;
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

  mqd_t main_q = mq_open(MAIN_QUEUE_PATH, O_RDONLY);

  thread_arg_t * thread_arg = malloc(sizeof(thread_arg_t));
  if (thread_arg == NULL) {
    // out of memory
    printf("out of mem\n");
    return 1;
  }
  thread_arg->main_q = main_q;

  pthread_t listen_thread_id;
  pthread_create(&listen_thread_id, NULL, listen_thread, (void *)thread_arg);


  int dont_halt = 1;
  char command_buffer[128];
  while (dont_halt) {
    // command line stuff here
    printf("cmd> ");
    fgets(command_buffer, 128, stdin);

    const char *killCmd = "stop\n";
    if (strcmp(killCmd, command_buffer) == 0) {
      // flush the main queue
      mq_close(main_q);
      mq_unlink(MAIN_QUEUE_PATH);
      dont_halt = 0;
    }
  }
  // kill other threads here
  pthread_kill(listen_thread_id, SIGKILL);
}
