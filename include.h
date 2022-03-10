#include <pthread.h>


// stuff here that each file may want
#define SHARED_MEM_LEN (32)
#define MAIN_QUEUE_PATH ("/procq1")

#define MAX_SEGMENTS_IN_PASS (100)

#define QUEUE_ID_LEN (7)

#define SLEEP_TIME (0)

typedef struct segment_metadata_block {
  int segment_id;
  short in_use;
  // put more stuff in here as we think of it
} seg_data_t;

typedef struct segment_metadata {
  seg_data_t *data_array;
  int seg_count;
  int used_seg_count;
  int seg_size;
  pthread_mutex_t lock;
} shared_memory_info;

#define IS_COMPRESSED_MSG (0)
