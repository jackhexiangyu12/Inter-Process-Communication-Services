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
#include <errno.h>

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

void print_error(int err) {
  if (err == EAGAIN)
    printf("eagain\n");
  if (err == EBADF)
    printf("ebadf\n");
  if (err == EINTR)
    printf("eintr\n");
  if (err == EINVAL)
    printf("einval\n");
  if (err == EMSGSIZE)
    printf("emsgsize\n");
  if (err == ETIMEDOUT)
    printf("etimedout\n");
  if (err == EACCES)
    printf("eacces\n");
  if (err == EINVAL)
    printf("einval\n");
  if (err == EMFILE)
    printf("emfile\n");
  if (err == ENOMEM)
    printf("enomem\n");

}

void release_segments(int available_segment_count, seg_data_t ***available_segments) {
  // should abstract this
  pthread_mutex_lock(&mem_info.lock);
  for (int i = 0; i < available_segment_count; i++) {
    // TODO: this needs to be a list of pointers to segment structs rather than ints
    (*available_segments)[i]->in_use = 0;
  }
  mem_info.used_seg_count -= available_segment_count;
  pthread_mutex_unlock(&mem_info.lock);
}

// need function where you give it a byte array, and it puts a proper shared memory message on q


int grab_segments(seg_data_t ***available_segments, unsigned long file_len) {
  // should be holding lock on mem_info

  // for starters, give half the free segments + 1


  // assumes we already hold a lock on mem_info

  // go thru all segments, grab as many as well can/need

  // TODO: this ^^

  // TODO: also set the "used" flag for each segment we grab


  // need to do this correctly





  db_print("about to grab segments here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;\n");
  while ((mem_info.seg_count - mem_info.used_seg_count) == 0) {
    // if the system has NO free segments, need to wait for some time
    //  yield the thread manually?
    // TODO: examine this behavior to see if it is desirable
    pthread_mutex_unlock(&mem_info.lock);
    sched_yield(); // this logic doesnt really work ?
    pthread_mutex_lock(&mem_info.lock);
    // TODO: actually implement this function properly without deadlocks
  }
  db_print("found segments to grab\n");


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


  mem_info.used_seg_count += amount_to_grab;


  return amount_to_grab;
}

void prep_segment_avail_metadata_msg(char *message_buffer, unsigned long file_len, int segments_available_count, seg_data_t ***available_segments) {
  db_print("pre prep message:: file_len: %lu, segs available (arr len): %d\n", file_len, segments_available_count);


  sprintf(message_buffer, "%d,%d,%lu,", mem_info.seg_size, segments_available_count, file_len);

    // now put the segment ids into the buffer, horribly (im sorry)

  // TODO: this gets buggy -- adds duplicate copies of itself on the end or something
  for (int i = 0; i < segments_available_count; i++) {
    char *tmp = calloc(MAX_MESSAGE_LEN, sizeof(char));
    memcpy(tmp, message_buffer, strlen(message_buffer)); // bc idk if this would work without copying
    sprintf(message_buffer, "%s%d,", tmp, (*available_segments)[i]->segment_id); // segfaulting on second client
    free(tmp);
  }
}

void *improved_check_clientq() {
  while(1) {
    // need to handle an ENTIRE client file tranfer from client to server here

    // first pop the client task queue
    /* db_printf("3 possible deadlock here idk ------------- ^^^^^^^^^^^^^^^^^^^^^          ==================\n"); */
    pthread_mutex_lock(&client_q.lock);

    if (queue_size(&client_q) <= 0) {
      pthread_mutex_unlock(&client_q.lock);
      continue;
    }

    task_node *curr_client = remove_head(&client_q);
    pthread_mutex_unlock(&client_q.lock);
    db_print("3 i guess no deadlock here or whatever\n");
    if (curr_client == NULL)
      /* sched_yield(); // helpful?  */
      continue;
    db_print("running a client task\n");

    // then need to grab the max amount of segments that we are allotted, but only enough for the file

    // want array of pointers, so use double pointer
    seg_data_t **available_segments = calloc(mem_info.seg_count, sizeof(seg_data_t *));

    pthread_mutex_lock(&mem_info.lock);
    // the grab segments line itself is segfaulting??
    db_print("gunna seg fault again...\n");
    /* seg_data_t ***tempvar = &available_segments; */
    /* db_print("gunna seg fault again merp... %d\n", tempvar); */
    db_print("about to compress and also print client q len !!@@\n");


    db_print("client q len: %d\n", client_q.size);
    db_print("uhhh some stuff  %d\n", curr_client->client);



    // the client list is being very wierd -- it shouldnt really be growing the way it is

    unsigned long thingy = curr_client->client->file_len;
    db_print("gunna seg fault again derp...%lu\n", thingy); // this is the culprit
    int available_segment_count = grab_segments(&available_segments, curr_client->client->file_len);
    pthread_mutex_unlock(&mem_info.lock);


    // now have array of segments that we can pass to the client

    // time for some unholy hacks bc im too lazy to code it correctly bc imagine having String.join() in a programming language standard library

    char *message_buffer = calloc(MAX_MESSAGE_LEN, sizeof(char));
    // TODO: ok to send zero as file len?
    prep_segment_avail_metadata_msg(message_buffer, curr_client->client->file_len, available_segment_count, &available_segments);

    // now the message buffer is good to be sent to the client

    // open up the send and listen queues

    char getQPath[128];
    sprintf(getQPath, "/%d", curr_client->client->get_queue_id);
    mqd_t client_mq_get = mq_open(getQPath, O_RDWR);

    char putQPath[128];
    sprintf(putQPath, "/%d", curr_client->client->put_queue_id);
    mqd_t client_mq_put = mq_open(putQPath, O_RDWR);

    db_print("both q ids: %d %d\n", curr_client->client->get_queue_id, curr_client->client->put_queue_id);


    // send the preliminary data message to client, then send the availability message

    db_print("prepped message: %s\n", message_buffer);
    db_print("client mq get path: %s\n", getQPath);
    db_print("client mq put path: %s\n", putQPath);
    sleep(SLEEP_TIME);


    if (curr_client->client->fresh) {
      // only send prelim if the task is fresh
      int ret_stat = mq_send(client_mq_get, message_buffer, strlen(message_buffer) + 1, 0);

      if (ret_stat == -1) {
        db_print(" messeage que is not working 1\n"); // emsgsize error
        // msg_len was greater than the mq_msgsize attribute of the message queue
        int err = errno;
        db_print("error codea: %d\n", err); // got 90
        print_error(err);

      } else {
        db_print("successfully sent a preliminary client q message\n");
      }


      // also need to allocate file buffer if fresh

      char *file_buffer = (char *) malloc(sizeof(char) * curr_client->client->file_len);

      curr_client->client->file_buffer = file_buffer;

      db_print("allocated file buffer\n");
      int segments_needed = (curr_client->client->file_len / mem_info.seg_size);
      if (curr_client->client->file_len % mem_info.seg_size != 0)
        segments_needed++;
      // keep track of how many more segments we need to grab
      int segments_remaining = segments_needed;
      curr_client->client->segments_remaining = segments_remaining;

      curr_client->client->total_segments_needed = segments_needed;

      // need to initalize the segment_index
      curr_client->client->segment_index = 0;
    }


    // setup is done

    // now do the actual main send and recv

    db_print("in clientq handler main part - sleeping then sending message\n");
    sleep(SLEEP_TIME);
    int ret_status = mq_send(client_mq_get, message_buffer, strlen(message_buffer) + 1, 0);

    if (ret_status == -1) {
      db_print(" messeage que is not working 2\n");

    } else {
      db_print("successfully sent a normal client q message\n");
    }

    // NOTE: this server thread works with the client library func send_data_to_server

    // after sending data to the client, wait for client to say its done packing the data
    // basically we are listening for an ACK -- dont verify the content bc i dont care about error checking
    db_print("about to listen for client ACK\n");


    // getting stuck here
    db_print("client put queue id: %s\n", putQPath);
    db_print("client get queue id: %s\n", getQPath);




    char tmp[MAX_MESSAGE_LEN];

    ret_status = mq_receive(client_mq_put, tmp, MAX_MESSAGE_LEN, 0);
    db_print("ACK?: %s\n", tmp); // the ack is being read incorrectly
    // TODO: error handling? dont assume the message is "OK" ?
    if (ret_status == -1) {
      db_print(" messeage que is not working 3\n");
      int err = errno;
      db_print("error codee: %d\n", err);

    } else {
      db_print("Message q is working\n");
    }

    // loop thru the segments that are allocated
    for (int j = 0; j < available_segment_count; j++) {
      if (curr_client->client->segments_remaining == 0)
        break; // yeet
      curr_client->client->segments_remaining--;
      // go thru each of the segments that we have,
      //  and copy the data into a special buffer

      // (j + i) * (segment size) is the index into the buffer for memcpy

      int segment_id = available_segments[j]->segment_id;
      char *sh_mem = (char *) shmat(segment_id, NULL, 0);

      int offset = (j + curr_client->client->segment_index) * mem_info.seg_size;
      if (curr_client->client->segments_remaining == 0) {
        int len = curr_client->client->file_len - offset;
        // TODO: I really hope i dereffed this properly
        db_print("about to segfault 1\n");
        memcpy(((curr_client->client->file_buffer)) + (offset), sh_mem, len);
      } else {
        db_print("about to segfault 2                            ---------------------------------------\n");
        // segfaults on the first round -- but whyyyyyyyyy
        memcpy(((curr_client->client->file_buffer)) + (offset), sh_mem, mem_info.seg_size);
        db_print("finished normal send copy\n");
      }
      // original:
      /* memcpy(file_buffer + ((j + i) * mem_info.seg_size), sh_mem, mem_info.seg_size); */

      // TODO: i really hope this is right ^^
    }

    db_print("finished send for loop thingy\n");

    /* for (int i = 0; i < segments_needed; i += available_segment_count) { } */



    // now cleanup

    mq_close(client_mq_put);
    mq_close(client_mq_get);
    free(message_buffer);

    curr_client->client->segment_index += available_segment_count;

    if (curr_client->client->segment_index >= curr_client->client->total_segments_needed) {
      db_print("cleaning up a complete send\n");
      // then we are complete
      curr_client->client->is_done = 1;

      // now ALL the file data is in the file_buffer
      // need to attach the file_buffer to a new compression task and put it on the queue
      ctask *comp_task = (ctask *) malloc(sizeof(ctask));
      comp_task->file_len = curr_client->client->file_len;
      comp_task->get_queue_id = curr_client->client->get_queue_id;
      comp_task->put_queue_id = curr_client->client->put_queue_id;
      /* comp_task->message_queue_id = curr_client->client->message_queue_id; */
      comp_task->file_buffer = curr_client->client->file_buffer;
      comp_task->compressed_len = 0;
      comp_task->fresh = 1; // very important
      comp_task->segment_index = 0; // also important




      task_node *comp_node = (task_node *) malloc(sizeof(task_node));
      comp_node->task = comp_task;
      comp_node->client = NULL;
      comp_node->next = NULL; // extremely important apparently

      log_print("\n[LOG]: completed recieving an entire file from a client, now placing task into compression task queue\n");

      pthread_mutex_lock(&task_q.lock);
      add_to_list(&task_q, comp_node);
      pthread_mutex_unlock(&task_q.lock);


      /* free(curr_client); */
      /* free(curr_client->client); // idk if this is safe*/
    } else {
      db_print("another round of send\n"); // blocking here

      curr_client->client->fresh = 0; // very very important

      db_print("1 probably about to deadlock ------------------------------------------------------=================               __________________-----\n");
      // put back on the client queue
      pthread_mutex_lock(&client_q.lock);
      db_print("magically aqured the lock\n");
      add_to_list(&client_q, curr_client);
      db_print("can i get uhhhh\n");
      pthread_mutex_unlock(&client_q.lock);


      db_print("1 did add to list for send --- no deadlock\n");
    }





    // TODO: free the node and the task object -- i think i covered that

    db_print("about to release the segments\n");
    release_segments(available_segment_count, &available_segments);
    free(available_segments);
  }
}


static void *improved_work_thread(void *arg) {
  // idk what args to use

  workthread_arg_t *thd_arg = (workthread_arg_t *) arg;

  pthread_mutex_lock(&client_q.lock);
  db_print("client q len %d\n", client_q.size);
  pthread_mutex_unlock(&client_q.lock);

  pthread_mutex_lock(&task_q.lock);
  db_print("task q len %d\n", task_q.size);
  pthread_mutex_unlock(&task_q.lock);

  // the queues are the correct lens, 0 and 1

  db_print("sleepy time before starting work\n");
  while (1) {
    pthread_mutex_lock(&task_q.lock);
    if (queue_size(&task_q) > 0) {
      db_print("got a compression task -----------------------------------------------------------------------------------------------------------------------\n");
      // then do stuff
      task_node *current_task = remove_head(&task_q);
      pthread_mutex_unlock(&task_q.lock);

      ctask *task = current_task->task; // does this copy into stack memory??

      /* // read the data from the segment */
      /* int segment_id = task->segment_id; */



      if (task->fresh) {
        /* sleep(10); */

        /* db_print("task info:  file len: %lu\n", current_task->task->file_len); */


        // then need to set up attributes and send preliminary message
        // TODO:
        // do snappy compress or something
        // put the compressed data back on the shared memory
        unsigned long compressed_len = 0;
        char *compressed_data_buffer = (char *) malloc(sizeof(char) * (snappy_max_compressed_length(task->file_len))); // make it too big in case -- // actually do what the spec says to do

        if (DO_COMPRESSION) {
          /* pthread_mutex_lock(&client_q.lock); */
          db_print("about to compress and also print client q len\n\n\n\n\n\n\n\n\n\n\n&&%%");

          db_print("client q len (sleepy again): %d\n", client_q.size);
          /* pthread_mutex_unlock(&client_q.lock); */
          int snappy_status = snappy_compress(thd_arg->env, (task->file_buffer), task->file_len, compressed_data_buffer, &compressed_len);
          log_print("\n[LOG]: completed compressing a file for a client\n");
          task->compressed_len = compressed_len;
          task->compressed_buffer = compressed_data_buffer;
        } else {
          memcpy(compressed_data_buffer, (task->file_buffer), task->file_len); // incase snappy fails
          compressed_len = task->file_len;
          task->compressed_len = compressed_len;
          task->compressed_buffer = compressed_data_buffer;
        }





        int segments_needed = (task->compressed_len / mem_info.seg_size);
        if (task->compressed_len % mem_info.seg_size != 0)
          segments_needed++;
        int segments_to_recv = segments_needed;


        task->total_segments_needed = segments_needed;
        task->segments_remaining = segments_to_recv;
        task->segment_index = 0;

      } else {
        // do nothing
      }

      //


      /* char *sh_mem = (char *) shmat(segment_id, NULL, 0); */

      // use:
      // task->file_buffer;
      // as the data for compression


      // grab free segments for data transfer
      seg_data_t **available_segments = calloc(mem_info.seg_count, sizeof(seg_data_t *));

      pthread_mutex_lock(&mem_info.lock);
      int available_segment_count = grab_segments(&available_segments, task->compressed_len);
      pthread_mutex_unlock(&mem_info.lock);



      // now need to tell client that data is compressed
      // might have to send back the data in multiple passes
      // ugh

      // TODO: check for deadlocks

      char getQPath[128];
      sprintf(getQPath, "/%d", task->get_queue_id);
      mqd_t client_mq_get = mq_open(getQPath, O_RDWR);

      char putQPath[128];
      sprintf(putQPath, "/%d", task->put_queue_id);
      mqd_t client_mq_put = mq_open(putQPath, O_RDWR);

      char *message_buffer = calloc(MAX_MESSAGE_LEN, sizeof(char));
      prep_segment_avail_metadata_msg(message_buffer, task->compressed_len, available_segment_count, &available_segments);





      if (task->fresh) {
        // need to send preliminary message
        db_print("about to send this message in work thread: %s\n", message_buffer);
        int ret_status = mq_send(client_mq_get, message_buffer, strlen(message_buffer) + 1, 0);

        if (ret_status == -1) {
          db_print(" messeage que is not working 4.1?\n");

        } else {
          db_print("Message q is working -- sent work thread prelim\n");
        }

      }




      /* for (int i = 0; i < segments_needed; i += available_segment_count) {} */
      // TODO: does this work

      // need to put the data onto the segments before signaling a transfer
      // basically the inverse of the check client q thread
      for (int j = 0; j < available_segment_count; j++) {
        if (task->segments_remaining == 0)
          break; // yeet
        task->segments_remaining--;
        int segment_id = available_segments[j]->segment_id;
        char *sh_mem = (char *) shmat(segment_id, NULL, 0);

        int offset = (j + task->segment_index) * mem_info.seg_size;
        if (task->segments_remaining == 0) {
          int len = task->compressed_len - offset;
          db_print("bruh 1\n");
          db_print("j: %d, seg index: %d, seg size: %d, seg id in use: %d, offset: %d, segments_remaining: %d, total segments needed %d\n", j, task->segment_index, mem_info.seg_size, segment_id, offset, task->segments_remaining, task->total_segments_needed);
          // get compressed buffer size

          db_print("compressed len: %lu\n", task->compressed_len);
          db_print("shmat return value: %d\n", sh_mem);
          db_print("size of a char %d\n", sizeof(char));
          char *dummy_buffer = (char *) malloc(sizeof(char) * (task->file_len));


          memcpy(sh_mem, (task->compressed_buffer) + (offset), len);
          /* memcpy(dummy_buffer, (task->compressed_buffer) + (offset), len); // no longer seg faults */

          free(dummy_buffer);
        } else {
          db_print("bruh 2\n");

          db_print("j: %d, seg index: %d, seg size: %d, seg id in use: %d, offset: %d, segments_remaining: %d, total segments needed %d\n", j, task->segment_index, mem_info.seg_size, segment_id, offset, task->segments_remaining, task->total_segments_needed);
          // get compressed buffer size

          db_print("compressed len: %lu\n", task->compressed_len);
          db_print("shmat return value: %d\n", sh_mem); // its -1, which means error
          char tbuf[64];
          sprintf(tbuf, "%d", sh_mem);
          int shmem_num = atoi(tbuf);
          db_print("%s\n",tbuf);
          int flip_flop = 0;
          if (shmem_num == -1) {
            db_print("got shmem error\n");
            int err = errno;
            print_error(err);
          }
          db_print("size of a char %d\n", sizeof(char));
          char *dummy_buffer = (char *) malloc(sizeof(char) * (task->file_len));

          memcpy(sh_mem, (task->compressed_buffer) + (offset), mem_info.seg_size);
          // sh_mem segfaults
          /* memcpy(dummy_buffer, (task->compressed_buffer) + (offset), mem_info.seg_size); */

          free(dummy_buffer);
        }

      }



      int ret_status = mq_send(client_mq_get, message_buffer, strlen(message_buffer) + 1, 0);

      if (ret_status == -1) {
        db_print(" messeage que is not working 4\n");

      } else {
        db_print("Message q is working -- sent work thread main msg\n");
      }

      // now wait for an ACK from the client -- no error handling implemented

      char tmp[MAX_MESSAGE_LEN];
      ret_status = mq_receive(client_mq_put, tmp, MAX_MESSAGE_LEN, 0);
      // TODO: error handling? dont assume the message is "OK" ?
      if (ret_status == -1) {
        db_print(" messeage que is not working 5\n");
        int err = errno;
        db_print("error codef: %d\n", err);
        print_error(err);
      } else {
        db_print("Message q is working - recv 5\n");
      }


      // now cleanup
      free(message_buffer);
      mq_close(client_mq_put);
      mq_close(client_mq_get);


      task->segment_index += available_segment_count;


      if (task->segment_index >= task->total_segments_needed) {
        // then we are totally complete
        free(task->compressed_buffer);
        free(task->file_buffer);

        log_print("\n[LOG]: completed sending a compressed file back to a client\n");

        // TODO: free everything else too
      } else {
        // put back on the queue
        task->fresh = 0; // very very important

        // TODO: anything else to handle????

        pthread_mutex_lock(&task_q.lock);
        add_to_list(&task_q, current_task);
        pthread_mutex_unlock(&task_q.lock);
      }



      // then free the segments

      release_segments(available_segment_count, &available_segments);
      free(available_segments);

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
    char recieve_buffer[8192]; // TODO: no hardcoding

    // blocking call
    int mq_ret = mq_receive(thread_arg->main_q, recieve_buffer, sizeof(recieve_buffer), NULL);

    if (mq_ret == -1) {
      db_print(" messeage que is not working 6\n");

    } else {
      db_print("Message q is working - got message in the listen thread\n");
    }

    // add to the client queue
    // message is %d%lu:, mqid, file_len

    char getQId[QUEUE_ID_LEN + 1];
    memcpy(getQId, recieve_buffer, QUEUE_ID_LEN);
    getQId[QUEUE_ID_LEN] = '\0';

    char putQId[QUEUE_ID_LEN + 1];
    memcpy(putQId, recieve_buffer + (QUEUE_ID_LEN * sizeof(char)), QUEUE_ID_LEN);
    putQId[QUEUE_ID_LEN] = '\0';

    int i = QUEUE_ID_LEN * 2;
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
    task->get_queue_id = atoi(getQId);
    task->put_queue_id = atoi(putQId);
    task->file_len = file_len;
    task->fresh = 1; // very very important
    db_print("initally parsed file len: %lu\n", file_len);


    node->client = task;
    node->next = NULL;
    node->task = NULL;
    //TODO:
    //add stuff to task?

    db_print("2 deadlocking?????? -------------------------_____________________________________________-------==========\n");


    log_print("\n[LOG]: recieved client request, adding to the client request handler queue\n");
    pthread_mutex_lock(&client_q.lock);
    add_to_list(&client_q, node);
    pthread_mutex_unlock(&client_q.lock);
    db_print("2 I guess no deadlocking\n");



    /* // TODO: add stuff to some pthread list */
    /* pthread_t request_handler_id; */
    /* pthread_create(&request_handler_id, NULL, handle_request, (void *)recieve_buffer); */

  }

  return NULL;
}


int main(int argc, char* argv[]) {
  /*

    set up message queue for use

    set up 30ms timer

    set up active queue and current task pointers


   */
  int inx;
  if (argc != 5){
    db_print("wrong Input usage\nUsage: --n_sms <num_segments> --sms_size <seg_size>");

    return;
  }

  // hardcode until get arg parsing
  int *segment_count = (int *)malloc(sizeof(int));
  int *segment_size_in_bytes = (int *)malloc(sizeof(int));
  *segment_count = atoi(argv[2]);
  *segment_size_in_bytes = atoi(argv[4]);

  mem_info.seg_count = *segment_count;
  mem_info.seg_size = *segment_size_in_bytes;
  db_print("segment size: %d", mem_info.seg_size);

  if (pthread_mutex_init(&mem_info.lock, NULL) != 0) {
    db_print("mutex init fail\n");
    return 1;
  }

  mem_info.data_array = malloc(sizeof(seg_data_t) * (*segment_count));
  if (mem_info.data_array == NULL) {
    db_print("out of mem\n");
    return 1;
  }

  for (int i = 0; i < (*segment_count); i++) {
    /* mem_info.data_array[i]; */
    int segment_id = shmget(IPC_PRIVATE, *segment_size_in_bytes, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    mem_info.data_array[i].segment_id = segment_id;
    mem_info.data_array[i].in_use = 0;
  }


  if (pthread_mutex_init(&task_q.lock, NULL) != 0) {
    db_print("mutex init fail\n");
    return 1;
  }

  if (pthread_mutex_init(&client_q.lock, NULL) != 0) {
    db_print("mutex init fail\n");
    return 1;
  }

  client_q.list_head = NULL;
  client_q.size = 0;
  task_q.list_head = NULL;
  task_q.size = 0;


  mqd_t setup_result = setup_main_q();
  if (setup_result == -1) {
    db_print("couldnt make the q\n");
    return 1;
  }

  mqd_t main_q = mq_open(MAIN_QUEUE_PATH, O_RDONLY);

  workthread_arg_t * wthread_arg = malloc(sizeof(workthread_arg_t));
  if (wthread_arg == NULL) {
    // out of memory
    db_print("out of mem\n");
    return 1;
  }

  thread_arg_t * lthread_arg = malloc(sizeof(thread_arg_t));
  if (lthread_arg == NULL) {
    // out of memory
    db_print("out of mem\n");
    return 1;
  }


  struct snappy_env *env = (struct snappy_env *) malloc(sizeof(struct snappy_env));
  snappy_init_env(env);
  wthread_arg->env = env;

  lthread_arg->main_q = main_q;

  pthread_t listen_thread_id;
  pthread_create(&listen_thread_id, NULL, listen_thread, (void *)lthread_arg);


  // changed to the improved design
  pthread_t check_client_thread_id;
  pthread_create(&check_client_thread_id, NULL, improved_check_clientq, NULL);

  if (DEBUG_WORK_MODE != 1) {
    pthread_t work_thread_id;
    pthread_create(&work_thread_id, NULL, improved_work_thread, (void *)wthread_arg);
  }



  int dont_halt = 1;
  char command_buffer[128];
  while (dont_halt) {
    // command line stuff here
    printf("cmd> ");
    fgets(command_buffer, 128, stdin);

    const char *killCmd = "stop\n";
    const char *print_segs = "psegs\n";
    const char *workCmd = "work\n";
    if (strcmp(killCmd, command_buffer) == 0) {
      // flush the main queue
      mq_close(main_q);
      mq_unlink(MAIN_QUEUE_PATH);
      dont_halt = 0;
    }

    if (strcmp(print_segs, command_buffer) == 0) {
      printf("---------------\n");
      pthread_mutex_lock(&mem_info.lock);
      for (int i = 0; i < mem_info.seg_count; i++) {
        printf("segment: %d, in_use? %d\n", i, mem_info.data_array[i].in_use);
      }
      pthread_mutex_unlock(&mem_info.lock);
      printf("_____--------------\n");
    }

    if (strcmp(workCmd, command_buffer) == 0) {
      printf("starting the work thread\n");
      if (DEBUG_WORK_MODE == 1) {
        pthread_t work_thread_id;
        pthread_create(&work_thread_id, NULL, improved_work_thread, (void *)wthread_arg);
      }
    }
  }
  snappy_free_env(wthread_arg->env);
  // kill other threads here
  pthread_kill(listen_thread_id, SIGKILL);
}
