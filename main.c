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


typedef struct workthread_arg {
  struct snappy_env *env;
} workthread_arg_t;


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


int grab_segments(seg_data_t ***available_segments, unsigned long file_len) {
  // for starters, give half the free segments + 1


  // assumes we already hold a lock on mem_info

  // go thru all segments, grab as many as well can/need

  // TODO: this ^^

  // TODO: also set the "used" flag for each segment we grab

  if ((mem_info.seg_count - mem_info.used_seg_count) == 0) {
    // if the system has NO free segments, need to wait for some time
    //  yield the thread manually?
    // TODO: examine this behavior to see if it is desirable
    pthread_mutex_unlock(&mem_info.lock);
    sched_yield();
    pthread_mutex_lock(&mem_info.lock);
  }


  // now to implement

  int amount_to_grab = (mem_info.seg_count - mem_info.used_seg_count) / 2;
  if (amount_to_grab == 0)
    amount_to_grab = 1;

  int amount_left_to_grab = amount_to_grab;
  int index = 0;
  for (int i = 0; i < mem_info.seg_count; i++) {
    // loop thru the segments,
    //  grab half the available ones, (or 1 if any are available)
    if (amount_left_to_grab > 0) {
      if (mem_info.data_array[i].in_use == 0) {
        // then we can snatch this one
        mem_info.data_array[i].in_use = 1;
        // TODO: pointer confusion -- make sure this works
        (*available_segments)[index] = &mem_info.data_array[i];
        index++;
        amount_left_to_grab--;
      }
    } else {
      break; // yeet
    }
  }





  return amount_to_grab;
}

void prep_segment_avail_metadata_msg(char **message_buffer, unsigned long file_len, int segments_available_count, seg_data_t ***available_segments) {
  sprintf(*message_buffer, "%d,%d,%lu,", mem_info.seg_size, segments_available_count, file_len);

    // now put the segment ids into the buffer, horribly (im sorry)

    for (int i = 0; i < segments_available_count; i++) {
      char tmp[2048];
      memcpy(tmp, *message_buffer, strlen(*message_buffer)); // bc idk if this would work without copying
      sprintf(*message_buffer, "%s%d,", tmp, (*available_segments)[i]->segment_id);
    }
}

// this thread is the client q handler
void *check_clientq() {
  while(1) {
    // need to handle an ENTIRE client file tranfer from client to server here

    // first pop the client task queue
    pthread_mutex_lock(&client_q.lock);
    task_node *curr_client = remove_head(&client_q);
    pthread_mutex_unlock(&client_q.lock);
    if (curr_client == NULL)
      /* sched_yield(); // helpful?  */
      continue;
    printf("got a client task\n");

    // then need to grab the max amount of segments that we are allotted, but only enough for the file

    // want array of pointers, so use double pointer
    seg_data_t **available_segments = calloc(mem_info.seg_count, sizeof(seg_data_t *));

    pthread_mutex_lock(&mem_info.lock);
    int available_segment_count = grab_segments(&available_segments, curr_client->client->file_len);
    pthread_mutex_unlock(&mem_info.lock);


    // now have array of segments that we can pass to the client

    // time for some unholy hacks bc im too lazy to code it correctly bc imagine having String.join() in a programming language standard library

    char *message_buffer = calloc(2048, sizeof(char));
    // TODO: ok to send zero as file len?
    prep_segment_avail_metadata_msg(&message_buffer, 0, available_segment_count, &available_segments);

    // now the message buffer is good to be sent to the client

    char messagePath[128];
    sprintf(messagePath, "/%d", curr_client->client->message_queue_id);
    mqd_t client_mq = mq_open(messagePath, O_RDWR);


    // send the preliminary data message to client, then send the availability message

    printf("%s\n", message_buffer);
    printf("client mq path: %s\n", messagePath);
    int ret_stat = mq_send(client_mq, message_buffer, strlen(message_buffer) + 1, 0);

    if (ret_stat == -1) {
      printf(" messeage que is not working 1\n");

    } else {
      printf("sent a preliminary client q message\n");
    }



    // setup is done
    // loop until the file is fully recieved
    // probably wont be able to figure out QoS

    char *file_buffer = (char *) malloc(sizeof(char) * curr_client->client->file_len);

    int segments_needed = (curr_client->client->file_len / mem_info.seg_size);
    if (curr_client->client->file_len % mem_info.seg_size != 0)
      segments_needed++;
    // keep track of how many more segments we need to grab
    int segments_to_recv = segments_needed;
    for (int i = 0; i < segments_needed; i += available_segment_count) { // TODO: should cover everything?
      int ret_status = mq_send(client_mq, message_buffer, strlen(message_buffer) + 1, 0);

      if (ret_status == -1) {
        printf(" messeage que is not working 2\n");

      } else {
        printf("sent a normal client q message\n");
      }

      // NOTE: this server thread works with the client library func send_data_to_server

      // after sending data to the client, wait for client to say its done packing the data
      // basically we are listening for an ACK -- dont verify the content bc i dont care about error checking
      char tmp[256];
      ret_status = mq_receive(client_mq, tmp, 11, NULL);
      printf("variable?: %s\n", tmp);
      // TODO: error handling? dont assume the message is "OK" ?
      if (ret_status == -1) {
        printf(" messeage que is not working 3\n");

      } else {
        printf("Message q is working\n");
      }


      for (int j = 0; j < available_segment_count; j++) {
        if (segments_to_recv == 0)
          break; // yeet
        segments_to_recv--;
        // go thru each of the segments that we have,
        //  and copy the data into a special buffer

        // (j + i) * (segment size) is the index into the buffer for memcpy

        int segment_id = available_segments[j]->segment_id;
        char *sh_mem = (char *) shmat(segment_id, NULL, 0);

        int offset = (j + i) * mem_info.seg_size;
        if (segments_to_recv == 0) {
          int len = curr_client->client->file_len - offset;
          memcpy(file_buffer + (offset), sh_mem, len);
        } else {
          memcpy(file_buffer + (offset), sh_mem, mem_info.seg_size);
        }
        // original:
        /* memcpy(file_buffer + ((j + i) * mem_info.seg_size), sh_mem, mem_info.seg_size); */

        // TODO: i really hope this is right ^^
      }
    }
    free(message_buffer);

    // now ALL the file data is in the file_buffer
    // need to attach the file_buffer to a new compression task and put it on the queue
    ctask *comp_task = (ctask *) malloc(sizeof(ctask));
    comp_task->file_len = curr_client->client->file_len;
    comp_task->message_queue_id = curr_client->client->message_queue_id;
    comp_task->file_buffer = &file_buffer;

    task_node *comp_node = (task_node *) malloc(sizeof(comp_node));
    comp_node->task = comp_task;

    pthread_mutex_lock(&task_q.lock);
    add_to_list(&task_q, comp_node);
    pthread_mutex_unlock(&task_q.lock);
  }
}


static void *work_thread(void *arg) {
  // idk what args to use

  workthread_arg_t *thd_arg = (workthread_arg_t *) arg;
  while (1) {
    pthread_mutex_lock(&task_q.lock);
    if (queue_size(&task_q) > 0) {
      printf("got a compression task\n");
      // then do stuff
      task_node *current_task = remove_head(&task_q);
      pthread_mutex_unlock(&task_q.lock);

      ctask task = *current_task->task;

      /* // read the data from the segment */
      /* int segment_id = task.segment_id; */


      /* char *sh_mem = (char *) shmat(segment_id, NULL, 0); */

      // use:
      // task.file_buffer;
      // as the data for compression

      // TODO:
      // do snappy compress or something
      // put the compressed data back on the shared memory
      unsigned long compressed_len = 0;
      char *compressed_data_buffer = (char *) malloc(sizeof(char) * task.file_len); // make it too big in case
      int snappy_status = snappy_compress(thd_arg->env, *(task.file_buffer), task.file_len, compressed_data_buffer, &compressed_len);
      /* memcpy(compressed_data_buffer, *(task.file_buffer), task.file_len); // incase snappy fails */
      compressed_len = task.file_len;


      // grab free segments for data transfer
      seg_data_t **available_segments = calloc(mem_info.seg_count, sizeof(seg_data_t *));

      pthread_mutex_lock(&mem_info.lock);
      int available_segment_count = grab_segments(&available_segments, compressed_len);
      pthread_mutex_unlock(&mem_info.lock);



      // now need to tell client that data is compressed
      // might have to send back the data in multiple passes
      // ugh

      // TODO: check for deadlocks

      char mqPath[128];
      sprintf(mqPath, "/%d", task.message_queue_id);
      mqd_t client_mq = mq_open(mqPath, O_RDWR);

      char *message_buffer = calloc(2048, sizeof(char));
      prep_segment_avail_metadata_msg(&message_buffer, compressed_len, available_segment_count, &available_segments);

      int segments_needed = (task.file_len / mem_info.seg_size);
      if (task.file_len % mem_info.seg_size != 0)
          segments_needed++;
      int segments_to_recv = segments_needed;
      for (int i = 0; i < segments_needed; i += available_segment_count) { // TODO: does this work

        // need to put the data onto the segments before signaling a transfer
        // basically the inverse of the check client q thread
        for (int j = 0; j < available_segment_count; j++) {
          if (segments_to_recv == 0)
            break; // yeet
          segments_to_recv--;
          int segment_id = available_segments[j]->segment_id;
          char *sh_mem = (char *) shmat(segment_id, NULL, 0);

          int offset = (j + i) * mem_info.seg_size;
          if (segments_to_recv == 0) {
            int len = task.file_len - offset;
            memcpy(sh_mem, (*(task.file_buffer)) + (offset), len);
          } else {
            memcpy(sh_mem, (*(task.file_buffer)) + (offset), mem_info.seg_size);
          }

        }



        int ret_status = mq_send(client_mq, message_buffer, strlen(message_buffer) + 1, 0);

        if (ret_status == -1) {
          printf(" messeage que is not working 4\n");

        } else {
          printf("Message q is working\n");
        }

        // now wait for an ACK from the client -- no error handling implemented

        char tmp[2048];
        ret_status = mq_receive(client_mq, tmp, strlen(tmp) + 1, 0);
        // TODO: error handling? dont assume the message is "OK" ?
        if (ret_status == -1) {
          printf(" messeage que is not working 5\n");

        } else {
          printf("Message q is working - recv 5\n");
        }
      }
      free(message_buffer);


      // then free the segment

      pthread_mutex_lock(&mem_info.lock);
      for (int i = 0; i < available_segment_count; i++) {
        // TODO: this needs to be a list of pointers to segment structs rather than ints
        available_segments[i]->in_use = 0;
      }
      mem_info.used_seg_count -= available_segment_count;
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

// the first part of the 3 thread design
// this listens to the main queue and then adds stuff to the client queue
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
      printf(" messeage que is not working 6\n");

    } else {
      printf("Message q is working - recv 6\n");
    }

    // add to the client queue
    // message is %d%lu:, mqid, file_len

    char mqId[QUEUE_ID_LEN + 1];
    memcpy(mqId, recieve_buffer, QUEUE_ID_LEN);
    mqId[QUEUE_ID_LEN] = '\0';

    int i = QUEUE_ID_LEN;
    int j = 0;
    char dataLenBuffer[64];
    while (recieve_buffer[i] != ':') {
      dataLenBuffer[j] = recieve_buffer[i];
      i++;
      j++;
    }
    dataLenBuffer[j] = '\0';

    char **f;
    unsigned long file_len = strtoul(dataLenBuffer, f, 10);

    task_node *node = (task_node *) malloc(sizeof(task_node));
    cltask *task = (cltask *) malloc(sizeof(cltask));

    task->is_done = 0;
    task->message_queue_id = atoi(mqId);
    task->file_len = file_len;


    node->client = task;
    //TODO:
    //add stuff to task?

    pthread_mutex_lock(&client_q.lock);
    add_to_list(&client_q, node);
    pthread_mutex_unlock(&client_q.lock);



    /* // TODO: add stuff to some pthread list */
    /* pthread_t request_handler_id; */
    /* pthread_create(&request_handler_id, NULL, handle_request, (void *)recieve_buffer); */

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
  int segment_count = 20;
  int segment_size_in_bytes = 16384;

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

  workthread_arg_t * wthread_arg = malloc(sizeof(workthread_arg_t));
  if (wthread_arg == NULL) {
    // out of memory
    printf("out of mem\n");
    return 1;
  }

  thread_arg_t * lthread_arg = malloc(sizeof(thread_arg_t));
  if (lthread_arg == NULL) {
    // out of memory
    printf("out of mem\n");
    return 1;
  }


  struct snappy_env *env = (struct snappy_env *) malloc(sizeof(struct snappy_env));
  snappy_init_env(env);
  wthread_arg->env = env;

  lthread_arg->main_q = main_q;

  pthread_t listen_thread_id;
  pthread_create(&listen_thread_id, NULL, listen_thread, (void *)lthread_arg);

  /* pthread_t work_thread_id; */
  /* pthread_create(&work_thread_id, NULL, work_thread, (void *)wthread_arg); */


  pthread_t check_client_thread_id;
  pthread_create(&check_client_thread_id, NULL, check_clientq, NULL);

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
  snappy_free_env(wthread_arg->env);
  // kill other threads here
  pthread_kill(listen_thread_id, SIGKILL);
}
