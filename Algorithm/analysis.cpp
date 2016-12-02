#include <stdio.h> 
#include <stdlib.h> // Needed for rand() and RAND_MAX
#include <math.h> // Needed for log()
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#define temp_buffer 8000

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    int count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

int main(){
	int i,j,n;
	int num_line=201;
	int num_field=17;
	double **power = (double**) calloc(num_line, sizeof(double*));
	for (i = 0; i < num_line; i++ ){
		power[i] = (double*) calloc(num_field, sizeof(double));
	}
	double **quality = (double**) calloc(num_line, sizeof(double*));
	for (i = 0; i < num_line; i++ ){
		quality[i] = (double*) calloc(num_field, sizeof(double));
	}
	int **latency = (int**) calloc(num_line, sizeof(int*));
	for (i = 0; i < num_line; i++ ){
		latency[i] = (int*) calloc(num_field, sizeof(int));
	}
	FILE *result_file;
	result_file = fopen("power.log", "r");
	char *new_line="\n";
	int row_counter;
	char temp[temp_buffer];
	for (i=0;i<num_line;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, result_file)!=NULL){
			tokens = str_split(temp, '\t');
			row_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						power[i][row_counter]=atof(*(tokens + n));
						row_counter++;
					}
					
					free(*(tokens + n));
				}
				free(tokens);
			}
		}
	}
	fclose(result_file);
	
	result_file = fopen("quality.log", "r");
	// char *new_line="\n";
	// int row_counter;
	// char temp[temp_buffer];
	for (i=0;i<num_line;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, result_file)!=NULL){
			tokens = str_split(temp, '\t');
			row_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						quality[i][row_counter]=atof(*(tokens + n));
						row_counter++;
					}
					
					free(*(tokens + n));
				}
				free(tokens);
			}
		}
	}
	fclose(result_file);
	
	result_file = fopen("latency.log", "r");
	// char *new_line="\n";
	// int row_counter;
	// char temp[temp_buffer];
	for (i=0;i<num_line;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, result_file)!=NULL){
			tokens = str_split(temp, '\t');
			row_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						latency[i][row_counter]=atoi(*(tokens + n));
						row_counter++;
					}
					
					free(*(tokens + n));
				}
				free(tokens);
			}
		}
	}
	fclose(result_file);
	// for (i=0;i<num_line;i++){
		// for (j=0;j<num_field;j++){
			// printf("%f\t",power[i][j]);
		// }
		// printf("\n");
	// }
	
	return 0;
}