void print_stuff();

typedef void* (*callback_t)(void *); // callback function with same arg format as pthread

unsigned char * sync_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len);
unsigned char * async_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len);
