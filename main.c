#include <stdio.h>

#include "snappy.h"
#include "task_queue.h"



int main() {
  printf("hello world\n");


  /*

    set up message queue for use

    set up 30ms timer

    set up active queue and current task pointers


   */

  int dont_halt = 1;
  while (dont_halt) {
    // put current task onto the active queue

    // check message queue for task
    // if no task, check active queue and switch to it
    // if message queue has a task, need to make new uthread for that task




    // this is just some stub stuff to make sure the makefile works
    active_q * tq = get_active_q();
    dont_halt = tq->stub;
  }
}
