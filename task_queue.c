#include <stdio.h>
#include <stdlib.h>
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

// need to transition this to circular
void add_to_list(object_q *q, task_node *t_node) {
  // should already have a lock on the list
  if (q->list_head == NULL) {
    // then set the lsit to the node
    q->list_head = t_node;
    t_node->next = t_node;
    q->size++;
    // print_list(list, 0);
    return;
  }

  if (q->size == 1) {
    q->size++;
    t_node->next = q->list_head;
    q->list_head->next = t_node;
    return;
  }


  // else, large size
  q->size++;

  // allocate a new node -- this new node will hold the head's data
  // the head will then get the new data
  // then bump the head pointer
  task_node *new_node = (task_node *) malloc(sizeof(task_node));
  new_node->client = q->list_head->client;
  new_node->task = q->list_head->task;

  new_node->next = q->list_head->next;

  q->list_head->next = new_node;

  // finish swaping head and next data

  q->list_head->client = t_node->client;;
  q->list_head->task = t_node->task;;

  q->list_head = q->list_head->next;

  free(t_node);

  return;
}


int queue_size(object_q *q) {
  return q->size;
}

task_node *remove_head(object_q *q) {
  // remove the head node

  if (q->list_head == NULL) {
    // return error -- TODO
    /* int *crash = NULL; */
    /* int hi = *crash; */
    return NULL;
  }


  if (q->size == 1) {
    q->size = 0;

    task_node *node = q->list_head;

    q->list_head = NULL;

    return node;
  }

  // else, variable size
  q->size--;

  // make a new dummy node
  task_node *return_node = (task_node *) malloc(sizeof(task_node));
  return_node->client = q->list_head->client;
  return_node->task = q->list_head->task;
  return_node->next = NULL;

  // put the second node's data into the head node

  task_node *second_node = q->list_head->next;

  q->list_head->client = second_node->client;
  q->list_head->task = second_node->task;

  // skip over the second node (that now has duplicate data)

  q->list_head->next = second_node->next;



  return return_node;
}
