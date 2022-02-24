#include <pthread.h>

typedef struct compression_task {
  // idk what metadata this needs - should copy from uthreads
  int stuff;
} ctask;

typedef struct task_list_node {
  ctask *task;
  struct task_list_node *next;
} task_node;


typedef struct __active_queue {

  ctask *current_task;
  int size; // size of the length of the linked list - dont count the current task
  task_node *list_head;

  pthread_mutex_t lock;
  int stub;
} active_q;


active_q *get_active_q();
void print_list(int grab_lock);
