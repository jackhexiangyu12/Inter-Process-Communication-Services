#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "client_library.h"

#include "snappy.h"
int multiple = 0;

typedef struct async_arg{
  unsigned long *compressed_len;
  char *compressed_file_buffer;
  int file_len;
  char *buffer;
} async_arg_t;
int flag = 0;
void *async( void *async_data){
  async_arg_t *data = (async_arg_t*)async_data;
  data->compressed_file_buffer = sync_compress(data->buffer, data->file_len, &data->compressed_len);
  flag = 1;
}

int call_compress( FILE **file1, int is_async, char *filename, int len_ ){
  int len = len_;

  char buf[len];
  if (multiple)
  buf[strcspn(buf, "\n")] = 0;
  char *f_name = strncpy(buf, filename, len);
  FILE *file = fopen(buf, "r");
  if (file == NULL){
    fprintf(stderr, "Unable to open file 2 %s\n", buf);
    return 1;
  }
  else{
    printf("correct \n");
  }

  unsigned char *buffer;
  unsigned long file_len;
  //file1 = fopen("compressed", "w+");


  //Get file length
  fseek(file, 0, SEEK_END);
  file_len=ftell(file);
  fseek(file, 0, SEEK_SET);

  //Allocate memory
  buffer = (char *) malloc(file_len);
  if (!buffer)
    {
      fprintf(stderr, "Memory error!");
      fclose(file);
      return 1;
    }

  fread(buffer,file_len,sizeof(unsigned char),file);
  fclose(file);

  char * compressed_file_buffer;
  unsigned long compressed_len = 0;
  pthread_t async_id;
  async_arg_t *async_data = malloc(sizeof(async_arg_t));
  if (is_async){
    async_data->buffer = buffer;
    async_data->compressed_len = &compressed_len;
    async_data->file_len = file_len;
    async_data->compressed_file_buffer = compressed_file_buffer;
    pthread_create(&async_id, NULL, async, (void*)async_data);
    //printf("This is printing before the program completion\n");
  }else{
    flag = 1;
    compressed_file_buffer = sync_compress(buffer, file_len, &compressed_len);
  }

  //Testing async call
  if (is_async)
  printf("This is printing before the program completion\n");
  if (is_async){
    pthread_join(async_id, NULL);
    compressed_file_buffer = async_data->compressed_file_buffer;
    file_len = async_data->file_len;
    compressed_len = async_data->compressed_len;
  }


  /* printf("just finished sync compress. here is the compressed data\n");
  //printf("%s\n", compressed_file_buffer);



  struct snappy_env *env = (struct snappy_env *) malloc(sizeof(struct snappy_env));
  snappy_init_env(env);

  char *uncomped = (char *) malloc(file_len);

  int snappy_status = snappy_uncompress(compressed_file_buffer, compressed_len, uncomped);


  //printf("uncompressed, trying to print the uncomped data\n");
  //printf("%s\n", uncomped);



  snappy_free_env(env);


  //fprintf(file1, "The text: %s\n", compressed_file_buffer); */


  return 0;
}

int main(int argc, char *argv[]) {
  printf("this is the client\n");
  print_stuff();
  if (argc != 5){
    printf("incorrect number of args\nargs format:  --file <input_file> --state <SYNC | ASYNC>");
    return 1;
  }

  char *arg1 = argv[1];
  char *arg2 = argv[2];
  char *arg3 = argv[3];
  char *arg4 = argv[4];
  int is_async = -1; // used to call async
  int multiple_files; //loop through file and reads
  if (strcmp(arg4, "ASYNC" ) == 0) is_async = 1;
  else if (strcmp(arg4, "SYNC" ) == 0) is_async = 0;
  if (is_async == -1){
    printf("incorrect number of args\nargs format:  --file <input_file> --state <SYNC | ASYNC>");
    return 1;
  }

  if (strcmp(arg1, "--file" ) == 0 && strcmp(arg3, "--state") == 0){
    multiple_files = 0;
  }
  else if (strcmp(arg1, "--files") == 0 && strcmp(arg3, "--state") == 0){
    multiple_files = 1;
    multiple = 1;
  }
  else{
    printf("incorrect number of args\nargs format:  --file <input_file> --state <SYNC | ASYNC>");
    return 1;
  }



  // read in file and put into buffer
  //Create and open file for send

  FILE *file, *file1;

  //open the file
  file = fopen(argv[2], "r");
  if (file == NULL) {
    fprintf(stderr, "Unable to open file %s\n", argv[2]);
    return 1;
  }
  if (multiple_files){
    FILE *fd_array[256];
    char *line;
    int len = 0;
    int counter = 0;
    while (getline(&line, &len, file) != -1)
    {
      //fd_array[counter] = fopen(line, "r");
      /*if(fd_array[counter] == NULL){
        fprintf(stderr, "Unable to open file 1 %s\n", line);
        return 1;
      }*/
      printf("line: %s\n", line);
      //printf("the len: %d\n", strlen(line));
      //fputs(len, stdout);
      call_compress(&fd_array[counter], is_async, line, strlen(line));
      counter++;
    }
  }
  else{

    call_compress(&file, is_async, arg2, strlen(arg2));
  }




}
