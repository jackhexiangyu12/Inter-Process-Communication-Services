#include <stdio.h>
#include "task_queue.h"



object_q *get_object_q() {
  // nothing function just for filler
  return NULL;
}




void print_list(object_q *q, int grab_lock) {
  if (grab_lock)
    pthread_mutex_lock(&q->lock);
  task_node *curr = q->list_head;

  if (curr == NULL) {
    // printf("<<<<EMPTY LIST>>>\n");
    printf("\n");
    if (grab_lock)
      pthread_mutex_unlock(&q->lock);
    return;
  }

  while (curr != NULL) {

    /* printf("%s ", curr->data); */

    curr = curr->next;
  }

  printf("\n");
  // printf("NIL\n");


  if (grab_lock)
    pthread_mutex_unlock(&q->lock);
}

void add_to_list(object_q *q, task_node *t_node) {
  // should already have a lock on the list
  if (q->list_head == NULL) {
    // then set the lsit to the node
    q->list_head = t_node;
    // print_list(list, 0);
    return;
  }

  if (q->list_head->next == NULL) {
    q->list_head->next = t_node;
    return;
  }

  // otherwise gotta find the end
  task_node *curr = q->list_head;
  while (curr->next != NULL) {
    /* dprint("looping"); */
    curr = curr->next;
  }
  // now curr is null

  curr->next = t_node;
  // print_list(list, 0);

}

int queue_size(object_q *q) {
  if (q->list_head == NULL)
    return 0;

  if (q->list_head->next == NULL)
    return 1;

  task_node *curr = q->list_head;
  int size = 1;
  while (curr->next != NULL) {
    curr = curr->next;
    size++;
  }
  return size;
}

task_node *remove_head(object_q *q) {
  // remove the head node

  if (q->list_head == NULL) {
    // return error -- TODO
    return NULL;
  }
  return NULL;
}
