void print_stuff();

typedef void* (*callback_t)(void *); // callback function with same arg format as pthread

int db_print(const char *format, ...); // for debug mode

char * sync_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len);
char * async_compress(unsigned char *data, unsigned long file_len, unsigned long *compressed_len);
