#include <stdio.h>
#include <stdlib.h>
#include "client_library.h"

#include "snappy.h"

int main(int argc, char *argv[]) {
  printf("this is the client\n");
  print_stuff();
  if (argc != 5){
    printf("incorrect number of args\nargs format: ./sample_app --file <input_file> --state <SYNC | ASYNC>");
    return;
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
    printf("incorrect number of args\nargs format: ./sample_app --file <input_file> --state <SYNC | ASYNC>");
    return;
  }

  if (strcmp(arg1, "--file" ) == 0 && strcmp(arg3, "--state") == 0){
    multiple_files = 0;
  }
  else if (strcmp(arg1, "--files") == 0 && strcmp(arg3, "--state") == 0){
    multiple_files = 1;
  }
  else{
    printf("incorrect number of args\nargs format: ./sample_app --file <input_file> --state <SYNC | ASYNC>");
    return;
  }



  // read in file and put into buffer
  //Create and open file for send
  char *filename = argv[1];
  unsigned char *buffer;
  unsigned long file_len;
  FILE *file, *file1;

  //open the file
  file = fopen(argv[2], "r+");
  file1 = fopen(argv[2], "w+");
  if (file == NULL) {
    fprintf(stderr, "Unable to open file %s\n", argv[1]);
    return 1;
  }

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


  unsigned long compressed_len = 0;
  char * compressed_file_buffer = sync_compress(buffer, file_len, &compressed_len);
  printf("just finished sync compress. here is the compressed data\n");
  printf("%s\n", compressed_file_buffer);



  struct snappy_env *env = (struct snappy_env *) malloc(sizeof(struct snappy_env));
  snappy_init_env(env);

  char *uncomped = (char *) malloc(file_len);

  int snappy_status = snappy_uncompress(compressed_file_buffer, compressed_len, uncomped);


  printf("uncompressed, trying to print the uncomped data\n");
  printf("%s\n", uncomped);



  snappy_free_env(env);


  fprintf(file1, "The text: %s\n", compressed_file_buffer);


  return 0;
}
