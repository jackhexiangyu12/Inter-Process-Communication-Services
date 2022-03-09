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
  attr.mq_msgsize = 50;
  char id[8]; // 7 long

  sprintf(id, "%d", randomId);

  char idPath[9];
  sprintf(idPath, "/%d", randomId);

  *return_q_ptr = mq_open(idPath, O_CREAT | O_RDWR, 0777, &attr);


  // open the shared queue for the server
  mqd_t main_server_q = mq_open(MAIN_QUEUE_PATH, O_WRONLY);

  char buf[2048];
  sprintf(buf, "%d%lu:", randomId, file_len); // trailing colon bc look at the parser in the server
  int len = strlen(buf);

  int mq_ret = mq_send(main_server_q, buf, len+1, 0);
  if (mq_ret == -1){
    printf(" messeage que is not working\n");

  }else{
    printf("Message q is working\n");
  }

  mq_close(main_server_q);


  // return the randomId just in case, also return value of the special queue that was created
  return randomId;
}

void send_data_to_server(unsigned long file_len, unsigned char *data, mqd_t *private_q) {

  // wait for segment id

  // TODO: get the segment ids in this protocol

  // <number of bytes in a segment>,<number of segment ids>,<first segment id>,<second segment id>

  // for now, hardcode the size, and only work with one segment id


  // then, when sending data to server, after putting the data on the segments,

  // send message in the following protocol:
  // <number of segments>,<first segment id>,<index of data sent on this segment>,...

  // client will have to look at the total file len, figure out the indexes for each chunk that it is
  //  sending, and then put all the compressed pieces back together






  /*

    temp starter code:

    wait for segment id

    put data on the segment

    send an OK message to server
   */

  char recv_buff[2048]; // need to figure out max size of message
  mq_receive(*private_q, recv_buff, sizeof(recv_buff), NULL);


  // just assume that the recv_buff can be parsed to a segment id

  int segment_id = atoi(recv_buff);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  memcpy(sh_mem, data, file_len);

  // send OK to server

  mq_send(*private_q, "OK", 3, NULL);

}

void receive_compressed_data(mqd_t *private_q, char **comp_data_buffer, unsigned long file_len) {
  // TODO: remove file len - I think?

  char buffer[64];
  // wait for compressed data signal from server
  mq_receive(*private_q, buffer, sizeof(buffer), NULL);

  // the signal will tell me that compressed data is ready to grab

  // it will come in this protocol -- same as the other one

  // <number of bytes in a segment>,<number of segment ids>,<first segment id>,<second segment id>

  // but for now, just assume that we get 1 segment id
  int segment_id = atoi(buffer);

  char *sh_mem = (char *) shmat(segment_id, NULL, 0);

  memcpy(*comp_data_buffer, sh_mem, file_len);


  // once I copy all the data into the compressed data buffer,
  //  tell the server that I got the data

  // will be complicated if needs multiple passes to send the data back

  char send_buff[64];
  sprintf(send_buff, "GOT");
  mq_send(*private_q, send_buff, strlen(send_buff) + 1, 0);

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
    printf(" messeage que is not working\n");

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

unsigned char * sync_compress(unsigned char *data, unsigned long file_len) {
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


  mqd_t private_q;
  int  private_q_id = establish_communicator_channel(file_len, &private_q);

  // now wait for the segment id(s) to come back from the server
  //  then put the data on the segment id
  // will have to make this more complicated once we are dealing with multiple segment ids at different times

  // we will send the ENTIRE file before trying to compress the data at all
  // this makes it way easier to implement and is allowed


  send_data_to_server(file_len, data, &private_q);

  // allocate a buffer for the compressed data -- its ok to allocate too much memory
  char *compressed_data_buffer = (char *) malloc(file_len);

  // we dont need threads for sync mode
  receive_compressed_data(&private_q, &compressed_data_buffer, file_len);

  // now destroy the message queue that was used to get the compressed file back
  mq_close(private_q);
  char idPath[9];
  sprintf(idPath, "/%d", private_q_id);
  mq_unlink(idPath);




  char *return_message_buffer;
  return get_compressed_data(return_message_buffer);
}


unsigned char * async_compress(unsigned char *data, unsigned long file_len) {
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
