#include <pthread.h>

typedef struct compression_task {
  // idk what metadata this needs - should copy from uthreads
  int stuff;
} ctask;

typedef struct client_task {
  // idk what metadata this needs - should copy from uthreads
  int message_queue_id;
  short is_done;
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
