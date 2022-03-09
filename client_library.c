#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <pthread.h>

#include <time.h>

#include "client_library.h"

#include "include.h"

// need procedure that takes in byte array and sync calls the server
// and then gets stuff in return
// returns pointer to compressed buffer

int establish_communicator_channel(unsigned long file_len, mqd_t *return_q_ptr) {
  // make initial request to server that contains file len and special msgQ id
  srand(time(0));

  int upper = 9999999;
  int lower = 1000000;
  int randomId = (rand() % (upper - lower + 1)) + lower;


  // create a new message queue for recieving a return message
  struct mq_attr attr;
  attr.mq_curmsgs = 0;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = 128;
  char id[8]; // 7 long

  sprintf(id, "%d", randomId);

  char idPath[9];
  sprintf(idPath, "/%d", randomId);
  printf("client mq path: %s\n", idPath);

  *return_q_ptr = mq_open(idPath, O_CREAT | O_RDWR, 0777, &attr);


  // open the shared queue for the server
  mqd_t main_server_q = mq_open(MAIN_QUEUE_PATH, O_WRONLY);

  char buf[2048];
  sprintf(buf, "%d%lu:", randomId, file_len); // trailing colon bc look at the parser in the server
  int len = strlen(buf);

  int mq_ret = mq_send(main_server_q, buf, len+1, 0);
  if (mq_ret == -1){
    printf(" messeage que is not working 7\n");

  }else{
    printf("Message q is working\n");
  }

  mq_close(main_server_q);


  // return the randomId just in case, also return value of the special queue that was created
  return randomId;
}


void parse_server_message(int **seg_array, char *message_buffer, unsigned long *file_len, int *seg_count, int *seg_size) {
  /* printf("inside parse server message --------------------------------\n"); */

  /* printf("hi 1\n"); */
  int i = 0;
  char seg_size_str[64];
  while (message_buffer[i] != ',') {
    seg_size_str[i] = message_buffer[i];
    i++;
  }
  seg_size_str[i] = '\0';
  *seg_size = atoi(seg_size_str);

  /* printf("seg size: %d\n", *seg_size); */
  i++; // skip over comma
  char seg_count_str[64];
  int indx = 0;
  while (message_buffer[i] != ',') {
    seg_count_str[indx] = message_buffer[i];
    i++;
    indx++;
  }
  seg_count_str[indx] = '\0';
  i++; // skip over comma

  *seg_count = atoi(seg_count_str);

  /* printf("seg count: %d\n", *seg_count); */

  char file_len_str[64];
  int flInx = 0;
  while (message_buffer[i] != ',') {
    file_len_str[flInx] = message_buffer[i];
    i++;
    flInx++;
  }
  file_len_str[flInx] = '\0';
  i++; // skip over comma

  char **f;
  unsigned long f_len = strtoul(file_len_str, f, 10);
  *file_len = f_len;

  /* int *seg_array = (int *) malloc(sizeof(int) * *seg_count); */
  /* int *seg_array = calloc(*seg_count, sizeof(int)); */
  /* printf("hi 4\n"); */

  int counter = 0;
  for (int index = 0; index < *seg_count; index++) {

    char seg_id_str[64];
    int ind = 0;
    while (message_buffer[i] != ',') {
      seg_id_str[ind] = message_buffer[i];
      i++;
      ind++;
    }
    seg_id_str[ind] = '\0';
    i++;

    /* printf("seg count: %d, counter: %d\n", *seg_count, counter); */
    /* printf("seg id str: %s\n", seg_id_str); */

    (*seg_array)[index] = atoi(seg_id_str);
    /* printf("array value: %d\n", (*seg_array)[index]); */
    counter++;
  }

  /* printf("about to return\n"); */
}

void send_data_to_server(unsigned long file_len, unsigned char *data, mqd_t *private_q) {

  // wait for segment id

  // TODO: get the segment ids in this protocol

  // <number of bytes in a segment>,<number of segment ids>,<first segment id>,...<last segment id>,

  // for now, hardcode the size, and only work with one segment id


  // then, when sending data to server, after putting the data on the segments,

  // send message in the following protocol:
  // <number of segments>,<first segment id>,<index of data sent on this segment>,...

  // client will have to look at the total file len, figure out the indexes for each chunk that it is
  //  sending, and then put all the compressed pieces back together


  char recv_buff[2048 * 4];
  int status = mq_receive(*private_q, recv_buff, sizeof(recv_buff), NULL);

  // get segment size from preliminary message

  // TODO: correct????
  int seg_count = 0;
  int seg_size = 0;
  unsigned long f_len = 0;
  printf("about to parse\n");
  printf("recv buff message: %s. message len: %lu\n", recv_buff, strlen(recv_buff));
  int *seg_array = calloc(MAX_SEGMENTS_IN_PASS, sizeof(int));
  parse_server_message(&seg_array, recv_buff, &f_len, &seg_count, &seg_size);
  printf("successful parse\n");

  // seg_size, seg_array, seg_count

  int segments_needed = (file_len / seg_size);
  if (file_len % seg_size != 0)
    segments_needed++;

  int segments_to_recv = segments_needed;

  int ii = 0;
  while (ii < segments_needed) {
    // blocking call - the server sends one each time data is ready to be accepted
    int stat = mq_receive(*private_q, recv_buff, sizeof(recv_buff), NULL);

    ///// reparse here
    seg_count = 0;
    seg_size = 0;
    parse_server_message(&seg_array, recv_buff, &f_len, &seg_count, &seg_size);
    printf("successful parse 2 ---------------------   ^^^^\n");



    // need to put data onto the segments before signaling an ACK to the server
    for (int j = 0; j < seg_count; j++) {
      if (segments_to_recv == 0)
        break; // yeet -- dont run over the limit or something
      segments_to_recv--;

      int segment_id = seg_array[j];
      char *sh_mem = (char *) shmat(segment_id, NULL, 0);

      int offset = ((j + ii) * seg_size);
      if (segments_to_recv == 0) {
        // TODO: if segments to recv == 0, then instead of seg_size, need to figure out the end of the buffer
        int len = file_len - offset;
        memcpy(sh_mem, data + (offset), len);
      } else {
        memcpy(sh_mem, data + (offset), seg_size);
      }

    }

    // now send ACK to the server
    struct mq_attr attr;
    mq_getattr(*private_q, &attr);
    /* attr.mq_curmsgs */
    printf("length of q: %d\n", attr.mq_curmsgs);
    printf("about to ack server\n");
    // is queue full???
    stat = mq_send(*private_q, "OK", 3, 0);
    printf("acked --------------------------------------*************\n");

    if (stat == -1) {
        printf(" messeage que is not working idk what num\n");

      } else {
        printf("Message q is working -- send OK\n");
      }

    ii += seg_count;
    printf("end of while loop client send\n");
  }
  free(seg_array);

  // TODO: anything else?
}

void receive_compressed_data(mqd_t *private_q, char **comp_data_buffer, unsigned long *compressed_len) {
  // server sends preliminary message for this transaction too -- just makes things easier
  // preliminary message has the segment size and the compressed len size
  char recv_buff[2048 * 4];
  int status = mq_receive(*private_q, recv_buff, sizeof(recv_buff), NULL);

  // get segment size from preliminary message

  // TODO: correct????
  int seg_count = 0;
  int seg_size = 0;
  int *seg_array = calloc(MAX_SEGMENTS_IN_PASS, sizeof(int));
  parse_server_message(&seg_array, recv_buff, compressed_len, &seg_count, &seg_size);
  printf("successful parse 3\n");
  // seg_size, seg_array, seg_count

  int segments_needed = (*compressed_len / seg_size);
  if (*compressed_len % seg_size != 0)
    segments_needed++;

  int segments_to_recv = segments_needed;

  int ii = 0;
  while (ii < segments_needed) {
    // blocking call - the server sends one each time data is ready to be accepted
    int stat = mq_receive(*private_q, recv_buff, sizeof(recv_buff), NULL);

    ///// reparse here
    seg_count = 0;
    seg_size = 0;
    parse_server_message(&seg_array, recv_buff, compressed_len, &seg_count, &seg_size);
    printf("successful parse 4\n");



    // need to put data onto the segments before signaling an ACK to the server
    for (int j = 0; j < seg_count; j++) {
      if (segments_to_recv == 0)
        break; // yeet -- dont run over the limit or something
      segments_to_recv--;

      int segment_id = seg_array[j];
      char *sh_mem = (char *) shmat(segment_id, NULL, 0);

      int offset = ((j + ii) * seg_size);
      if (segments_to_recv == 0) {
        // TODO: if segments to recv == 0, then instead of seg_size, need to figure out the end of the buffer
        int len = *compressed_len - offset;
        memcpy(*comp_data_buffer + (offset), sh_mem, len);
      } else {
        memcpy(*comp_data_buffer + (offset), sh_mem, seg_size);
      }

    }

    // now send ACK to the server
    stat = mq_send(*private_q, "OK", 3, 0);

    ii += seg_count;
  }

  free(seg_array);
}


void send_original_file(unsigned char *data, unsigned long file_len, mqd_t *return_q_ptr, int *return_q_id) {
  mqd_t mq_snd_open = mq_open(MAIN_QUEUE_PATH, O_WRONLY);
  // make shared memory here

  /* printf("virtual address: %d\n", (int) sh_mem); */

  //char * text = "hello there\n";


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

  *return_q_id = randomId;
  sprintf(id, "%d", randomId);

  char idPath[9];
  sprintf(idPath, "/%d", randomId);

  *return_q_ptr = mq_open(idPath, O_CREAT | O_RDWR, 0777, &attr);

  // need to error check to see that this queue is new

  char buf[8192]; // figure out what this is for

  sprintf(buf, "%d%lu", randomId, file_len); // id, then stringified segment id
  int len = strlen(buf);
  printf("the q id: %d\n", randomId);
  printf("the whole message: %s\n", buf);

  int mq_ret = mq_send(mq_snd_open, buf, len+1, 0);
  if (mq_ret == -1){
    printf(" messeage que is not working 8\n");

  }else{
    printf("Message q is working\n");
  }

  // wait for reply with segment id to be used




}

unsigned char *get_compressed_data(char *return_message_buffer) {
  // size:segment_id in the return_message_buffer

  // need to grab the compressed len and the segment id from the message
  int i =  0;
  char compressedLenBuffer[64];
  while (return_message_buffer[i] != ':') {
    // then we know stuff
    compressedLenBuffer[i] = return_message_buffer[i];
    i++;
  }
  compressedLenBuffer[i] = '\0';

  char **f;
  unsigned long compressed_len = strtoul(compressedLenBuffer, f, 10);

  int segment_id = atoi(return_message_buffer);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  // memcpy stuff from shared mem to the compressed data buffer
  // TODO:

  unsigned char *compressed_data_buffer = (char *) malloc(compressed_len);


  memcpy(compressed_data_buffer, sh_mem, compressed_len);


  return compressed_data_buffer;
}

unsigned char * sync_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len) {
  // qclient.c code here
  // but not the file reader part

  /*

    client steps:
    client make request to server, telling it what message q to use (also send file len info, but thats a later problem)

    client waits for the segment id(s) back from the server on the special message q

    client puts data onto the segment(s)

    client sends DATA_READY signal to server on the special message queue

    client waits for compressed data from the server

    client gets the data and copies it into a buffer

    client tells server GOT_DATA


   */


  mqd_t *private_q = malloc(sizeof(mqd_t));
  // TODO: check for null
  int  private_q_id = establish_communicator_channel(file_len, private_q);
  /* int mq_ret = mq_send(*private_q, "hello", 6, 0); */
  /* if (mq_ret == -1){ */
  /*   printf(" messeage que is not working tmp\n"); */

  /* }else{ */
  /*   printf("Message q is working\n"); */
  /* } */

  // now wait for the segment id(s) to come back from the server
  //  then put the data on the segment id
  // will have to make this more complicated once we are dealing with multiple segment ids at different times

  // we will send the ENTIRE file before trying to compress the data at all
  // this makes it way easier to implement and is allowed


  send_data_to_server(file_len, data, private_q);

  // allocate a buffer for the compressed data -- its ok to allocate too much memory
  char *compressed_data_buffer = (char *) malloc(file_len);

  // we dont need threads for sync mode
  receive_compressed_data(&private_q, &compressed_data_buffer, compressed_len);

  // now destroy the message queue that was used to get the compressed file back
  printf("about to close and destroy the client q\n");
  mq_close(*private_q);
  char idPath[9];
  sprintf(idPath, "/%d", private_q_id);
  mq_unlink(idPath);




  char *return_message_buffer;
  return get_compressed_data(return_message_buffer);
}


unsigned char * async_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len) {
  // TODO: right now this is the same as sync mode


  mqd_t return_q;
  int  return_q_id = 0;
  send_original_file(data, file_len, &return_q, &return_q_id);

  char return_message_buffer[64]; // should probably unify these sizes

  // now do sync call to wait to receive a message back
  printf("about to read from the return q\n");
  mq_receive(return_q, return_message_buffer, sizeof(return_message_buffer), NULL);
  /* printf("message queue has: %s\n", return_message_buffer); */

  // now destroy the message queue that was used to get the compressed file back
  mq_close(return_q);
  char idPath[9];
  sprintf(idPath, "/%d", return_q_id);
  mq_unlink(idPath);


  return get_compressed_data(return_message_buffer);

}

void print_stuff() {
  printf("this is stuff\n");
}
