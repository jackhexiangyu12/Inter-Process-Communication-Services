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
#include "client_library.h"
#include "include.h"

typedef struct thread_arg_payload {
  mqd_t main_q;
} thread_arg_t;


shared_memory_info mem_info;

object_q task_q;
object_q client_q;


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

static void *work_thread(void *arg) {
  // idk what args to use
  while (1) {
    pthread_mutex_lock(&task_q.lock);
    if (queue_size(&task_q) > 0) {
      // then do stuff

      task_node *current_task = remove_head(&task_q);
      pthread_mutex_unlock(&task_q.lock);

      ctask task = *current_task->task;

      // read the data from the segment
      int segment_id = task.segment_id;


      char *sh_mem = (char *) shmat(segment_id, NULL, 0);

      // TODO:
      // do snappy compress or something
      // put the compressed data back on the shared memory




      // tell the client that the segment is compressed

      char mqPath[128];
      sprintf(mqPath, "/%d", task.message_queue_id);

      char return_message_buf[128];
      sprintf(return_message_buf, "%d%d", IS_COMPRESSED_MSG, segment_id);

      mqd_t client_mq = mq_open(mqPath, O_RDWR);

      int ret_status = mq_send(client_mq, return_message_buf, strlen(return_message_buf) + 1, 0);

      // wait for client to respond with DONE

      // blocking
      ret_status = mq_receive(client_mq, return_message_buf, sizeof(return_message_buf), NULL);
      // TODO: parse message for error handling
      // for now, assume any message means DONE

      // then free the segment

      pthread_mutex_lock(&mem_info.lock);
      mem_info.data_array[task.segment_index].in_use = 0;
      mem_info.used_seg_count--;
      pthread_mutex_unlock(&mem_info.lock);

    } else {
      pthread_mutex_unlock(&task_q.lock);
    }
  }

  // pop the work q
  // if nothing, keep looping
  // if something, do work until done
  // put compressed data on the segment -- overwrite uncompressed data
  // once done, tell the client that the segment is compressed
  // the client then puts that segment into a buffer
  // the client then tells us that it read the segment
  // we then free the segment to be used by other clients


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


  // hardcode until get arg parsing
  int segment_count = 5;
  int segment_size_in_bytes = 1028;

  mem_info.seg_count = segment_count;
  mem_info.seg_size = segment_size_in_bytes;

  if (pthread_mutex_init(&mem_info.lock, NULL) != 0) {
    printf("mutex init fail\n");
    return 1;
  }

  mem_info.data_array = malloc(sizeof(seg_data_t) * segment_count);
  if (mem_info.data_array == NULL) {
    printf("out of mem\n");
    return 1;
  }

  for (int i = 0; i < segment_count; i++) {
    /* mem_info.data_array[i]; */
    int segment_id = shmget(IPC_PRIVATE, segment_size_in_bytes, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    mem_info.data_array[i].segment_id = segment_id;
    mem_info.data_array[i].in_use = 0;
  }


  if (pthread_mutex_init(&task_q.lock, NULL) != 0) {
    printf("mutex init fail\n");
    return 1;
  }

  if (pthread_mutex_init(&client_q.lock, NULL) != 0) {
    printf("mutex init fail\n");
    return 1;
  }

  client_q.list_head = NULL;
  task_q.list_head = NULL;


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
