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
