#include "common.h"
#define temp_buffer 8000
#define num_shard 16

int read_trace();
char** str_split(char* a_str, const char a_delim);
void rand_array_with_fix_sum(int* array, int length, int sum);