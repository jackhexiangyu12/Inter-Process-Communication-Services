#include <pthread.h>

typedef struct compression_task {
  // idk what metadata this needs - should copy from uthreads
  /* int message_queue_id; // not anymore */
  int get_queue_id;
  int put_queue_id;
  int segment_id; // dont need anymore
  int data_len; // wot
  int segment_index; // not sure if this is needed anymore
  unsigned long file_len;
  char *file_buffer; // points to a buffer for the file, in heap space
} ctask;

typedef struct client_task {
  // idk what metadata this needs - should copy from uthreads
  int get_queue_id;
  int put_queue_id;
  /* int message_queue_id; // for the client's private message queue */
  unsigned long file_len; // of the file being compressed
  short is_done; // not sure if this is needed anymore
  short fresh; // init to 1 -- set to zero after sending prelim
  char *file_buffer; // points to a buffer for the file, in heap space

  int segment_index; // acts as the 'i' index from the main loop
  int total_segments_needed;
  int segments_remaining; // keep track of how many left
} cltask;


typedef struct task_list_node {
  cltask *client;
  ctask *task;
  struct task_list_node *next;
} task_node;


typedef struct __active_queue {
  task_node *list_head;
  pthread_mutex_t lock;
} object_q;


object_q *get_active_q();
void print_list(object_q *q, int grab_lock);
void add_to_list(object_q *q, task_node *t_node);
int queue_size(object_q *q);
task_node *remove_head(object_q *q);
