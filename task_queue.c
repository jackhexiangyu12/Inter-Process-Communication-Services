#include <stdio.h>
#include "task_queue.h"

active_q q = {0}; // stub it out for now



active_q *get_active_q() {
  // nothing function just for filler
  return &q;
}




void print_list(int grab_lock) {
  if (grab_lock)
    pthread_mutex_lock(&q.lock);
  task_node *curr = q.list_head;

  if (curr == NULL) {
    // printf("<<<<EMPTY LIST>>>\n");
    printf("\n");
    if (grab_lock)
      pthread_mutex_unlock(&q.lock);
    return;
  }

  while (curr != NULL) {

    /* printf("%s ", curr->data); */

    curr = curr->next;
  }

  printf("\n");
  // printf("NIL\n");


  if (grab_lock)
    pthread_mutex_unlock(&q.lock);
}

void add_to_list(task_node *t_node) {
  // should already have a lock on the list
  if (q.list_head == NULL) {
    // then set the lsit to the node
    q.list_head = t_node;
    // print_list(list, 0);
    return;
  }

  if (q.list_head->next == NULL) {
    q.list_head->next = t_node;
    return;
  }

  // otherwise gotta find the end
  task_node *curr = q.list_head;
  while (curr->next != NULL) {
    /* dprint("looping"); */
    curr = curr->next;
  }
  // now curr is null

  curr->next = t_node;
  // print_list(list, 0);

}
