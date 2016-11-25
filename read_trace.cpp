#include "read_trace.h"
#include "template_impl.tpp"
/********************************************
read trace from three files in trace/
1. merge.log, which has query string, agg latency(log from solr), 
per-shard service time (num_shard) and per-shard posting length (num_shard)
2. query_result.log, which has query string, agg latency(log from http response), 
and shard index of every match document.
3. query_scores.log, which has query string and scores for every match document.

First, it reads the line for each files, and set the line to the minimum of three files.
and it read the files into each array.

/********************************************
/********************************************
********************************************/


char **query; // query string from query_result.log
int **result_shard; // shard index from query_result.log
double *Agg_time; // agg latency frin query_result.log
int **shard_doc_count; // per-shard matched docs count

char **scores_query; //query string from query_scores.log
double **scores; // per matched doc scores from query_scores.log
double **shard_scores; //per-shard matched scores

char **time_query; //query string from merge.log
double **query_service_time; // per-shard service time from merge.log
double *query_agg_time; // agg latency frin merge.log
int **query_posting_length; // per-shard posting length from merge.log

int row;
int num_line;

int read_trace(){
	int i,j,k,n;
	// get line count for query_result.log
	FILE *fp;
	char path[1035];
	
	fp = popen("wc -l trace/query_result.log | awk '{print $1}'", "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path)-1, fp) != NULL) {
		num_line=atoi(path);
		// printf("%d\n", num_line);
	}

	/* close */
	pclose(fp);
	
	// get row count for query_result.log
	char path_row[1035];
	
	fp = popen("cat trace/query_result.log | awk '{if(q==null){q=NF} else {if(NF!=q) {q=-1; exit; }}} END {print q}'", "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(path_row, sizeof(path_row)-1, fp) != NULL) {
		row=atoi(path_row)-2;
		// printf("%d\n", row);
	}

	/* close */
	pclose(fp);
	
	if(row<0){
		printf("negative row count\n");
		return -1;
	}
	
	if (row<top_k){
		printf("top k less than row %d\t%d\n",top_k,row);
		return -1;
	
	}
	
	/* Get the line count of the timing result document */
	int num_line_time;
	
	FILE *fp_time;
	char path_time[1035];
	
	fp_time = popen("wc -l trace/merge.log | awk '{print $1}'", "r");
	if (fp_time == NULL) {
		printf("Failed to run command\n" );
		return -1;
	}
	
	/* Read the output a line at a time - output it. */
	while (fgets(path_time, sizeof(path_time)-1, fp_time) != NULL) {
		num_line_time=atoi(path_time);
		// printf("%d\n", num_line_time);
	}
	
	/* close */
	pclose(fp_time);
	
	int num_line_scores;
	/* Get the line count of the query score document */
	
	// get line count for query_scores.log
	FILE *fp_scores;
	char path_scores[1035];
	
	fp_scores = popen("wc -l trace/query_scores.log | awk '{print $1}'", "r");
	if (fp_scores == NULL) {
		printf("Failed to run command\n" );
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(path_scores, sizeof(path_scores)-1, fp_scores) != NULL) {
		num_line_scores=atoi(path_scores);
		// printf("%d\n", num_line);
	}

	/* close */
	pclose(fp_scores);
	
	// get row count for query_scores.log
	char path_socre_row[1035];
	
	fp_scores = popen("cat trace/query_scores.log | awk '{if(q==null){q=NF} else {if(NF!=q) {q=-1; exit; }}} END {print q}'", "r");
	if (fp_scores == NULL) {
		printf("Failed to run command\n" );
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(path_socre_row, sizeof(path_socre_row)-1, fp_scores) != NULL) {
		if(atoi(path_socre_row)-1!=row){
			printf("query_result.log and query_scores.log have different row count %d,%d\n",row,atoi(path_socre_row)-1);
			return -1;
		}
		// printf("%d\n", row);
	}

	/* close */
	pclose(fp_scores);
	
	int min_line=0;
	if(num_line_time!=num_line || num_line_scores!=num_line) {
		error("line count for merge.log, query_result.log and query_scores.log does not match %d,%d,%d\n",num_line,num_line_time,num_line_scores);
		if(num_line_time<num_line) {
			if(num_line_scores<num_line_time) {
				min_line=num_line_scores;
			}else{
				min_line=num_line_time;
			}
		}
		else {
			if(num_line_scores<num_line) {
				min_line=num_line_scores;
			}else{
				min_line=num_line;
			}
		}
		num_line=min_line;
		num_line_time=min_line;
		num_line_scores=min_line;
	}
	
	/* End getting the line/row count*/
		
	
	/****process the query result***/
	
	query = create_2D_array<char>(num_line,200,0); // query string
	result_shard = create_2D_array<int>(num_line,200,0); // result top N doc shard index
	// int **shard = (int**) calloc(num_line, sizeof(int*));
	// for (i = 0; i < num_line; i++ ){
		// shard[i] = (int*) calloc(200, sizeof(int));
	// }
	Agg_time = create_1D_array<double>(num_line,0); // query latency (get from http content)
	// float *Agg_time = (float*) calloc(num_line, sizeof(float));
	
	char temp[temp_buffer];
	
	FILE *result_file;
	result_file = fopen("trace/query_result.log", "r");
	char const *new_line="\n";
	int row_counter;
	for (i=0;i<num_line;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, result_file)!=NULL){
			tokens = str_split(temp, '\t');
			row_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						if(n==0){
							strcpy(query[i],*(tokens + n));
							// printf("%s ",query[i]);
						}else if (n==1){
							Agg_time[i]=atof(*(tokens + n));
							// printf("%f ",time[i]);
						}else{
							result_shard[i][row_counter]=atoi(*(tokens + n));
							// printf("%d ", shard[i][row_counter]);
							row_counter++;
						}
					}
					
					free(*(tokens + n));
				}
				if(row_counter!=row){
					printf("row count %d != %d\n",row_counter,row);
					return 0;
				}
				// printf("\n");
				
				free(tokens);
			}
		}
	}
	fclose(result_file);
	shard_doc_count = create_2D_array<int>(num_line,num_shard,0); // per-shard match doc count
	int temp_sum;
	// int **shard_doc_count = (int**) calloc(num_line, sizeof(int*));
	
	// for (i = 0; i < num_line; i++ ){
		// shard_doc_count[i] = (int*) calloc(num_shard, sizeof(int));
	// }
	
	
	for(i=0;i<num_line;i++){
		for(k=0;k<num_shard;k++){
			shard_doc_count[i][k]=0;
		}
		temp_sum=0;
		if(result_shard[i][0]==0){ // does not log single term query, random generate shard doc count
			int* kkkkk=(int*) calloc(num_shard, sizeof(int));
			rand_array_with_fix_sum(kkkkk,num_shard,top_k);
			
			for(k=0;k<num_shard;k++){
				shard_doc_count[i][k]=kkkkk[k];
				// printf("%d\t",shard_doc_count[i][k]);
			}
			// printf("\n");
			free(kkkkk);
		}else{
			for(j=0;j<top_k;j++){
				shard_doc_count[i][result_shard[i][j]-1]++;
				// printf("%d\t",shard[i][j]);
			}
		}
		// printf("\n");
		for(k=0;k<num_shard;k++){
			// printf("%d\t",shard_doc_count[i][k]);
			
		}
		// printf("\n");
		
	}
	/****finish process the query result***/
	// printf("query_result.log complete\n");
	
	
	
		
	
	/****process the query scores***/
	scores_query = create_2D_array<char>(num_line_scores,200,0); // query string
	// char **scores_query = (char**) calloc(num_line_scores, sizeof(char*));
	// for (i = 0; i < num_line_scores; i++ ){
		// scores_query[i] = (char*) calloc(200, sizeof(char));
	// }
	scores = create_2D_array<double>(num_line_scores,200,0); // result doc scores
	// double **scores = (double**) calloc(num_line_scores, sizeof(double*));
	// for (i = 0; i < num_line; i++ ){
		// scores[i] = (double*) calloc(200, sizeof(double));
	// }
		
	FILE *scores_file;
	scores_file = fopen("trace/query_scores.log", "r");
	
	for (i=0;i<num_line_scores;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, scores_file)!=NULL){
			tokens = str_split(temp, '\t');
			row_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						if(n==0){
							strcpy(scores_query[i],*(tokens + n));
							// printf("%s ",query[i]);
						}else{
							scores[i][row_counter]=atof(*(tokens + n));
							// printf("%d ", shard[i][row_counter]);
							row_counter++;
						}
					}
					
					free(*(tokens + n));
				}
				if(row_counter!=row){
					printf("row count %d != %d\n",row_counter,row);
					return -1;
				}
				// printf("\n");
				
				free(tokens);
			}
		}
	}
	fclose(scores_file);
	shard_scores = create_2D_array<double>(num_line_scores,num_shard,0); // per-shard match total scores
	// double **shard_scores = (double**) calloc(num_line_scores, sizeof(double*));
	
	// for (i = 0; i < num_line; i++ ){
		// shard_scores[i] = (double*) calloc(num_shard, sizeof(double));
	// }
	
	
	for(i=0;i<num_line_scores;i++){
		for(k=0;k<num_shard;k++){
			shard_scores[i][k]=0;
		}
		
		for(j=0;j<top_k;j++){
			shard_scores[i][result_shard[i][j]-1]+=scores[i][j];
			// printf("%d\t",shard[i][j]);
		}
		
		// printf("\n");
		for(k=0;k<num_shard;k++){
			// printf("%d\t",shard_doc_count[i][k]);
			
		}
		// printf("\n");
		
	}
	/****finish process the query scores***/
	
	/***process query time***/
	time_query = create_2D_array<char>(num_line_time,200,0); // query string
	// char **time_query = (char**) calloc(num_line_time, sizeof(char*));
	// for (i = 0; i < num_line_time; i++ ){
		// time_query[i] = (char*) calloc(200, sizeof(char));
	// }
	query_service_time = create_2D_array<double>(num_line_time,200,0); // per-shard query service time
	// float **process_time = (float**) calloc(num_line_time, sizeof(float*));
	// for (i = 0; i < num_line_time; i++ ){
		// process_time[i] = (float*) calloc(200, sizeof(float));
	// }
	query_agg_time = create_1D_array<double>(num_line_time,0); // aggregator latency (from solr)
	query_posting_length = create_2D_array<int>(num_line_time,200,0); // per-shard query posting length
	FILE *time_file;
	time_file = fopen("trace/merge.log", "r");
	// char *new_line="\n";
	int shard_counter,posting_counter;
	char const *error_str="error\n";
	for (i=0;i<num_line_time;i++){
		char** tokens;
		if(fgets(temp, temp_buffer, time_file)!=NULL){
			tokens = str_split(temp, ' ');
			shard_counter=0;
			posting_counter=0;
			if (tokens){
				for (n = 0; *(tokens + n); n++){	
					if(strcmp(*(tokens + n),new_line)){
						if(n==0){
							if(strcmp(*(tokens + n),error_str)){
								// printf("%s\n",*(tokens + n));
								strcpy(time_query[i],*(tokens + n));
							}else{
								sprintf(time_query[i],"error");
								// printf("line %d hit error\n",i+1);
								shard_counter=num_shard;
								posting_counter=num_shard;
								
							}
						} else if(n==1){
							query_agg_time[i]=atof(*(tokens + n));
						} else if(n<=num_shard+1){
							query_service_time[i][shard_counter]=atof(*(tokens + n));
							shard_counter++;
						} else {
							query_posting_length[i][posting_counter]=atoi(*(tokens + n));
							// printf("%d ", query_posting_length[i][posting_counter]);
							posting_counter++;
						}
					}
					
					free(*(tokens + n));
				}
				// printf("\n");
				if(shard_counter!=num_shard || posting_counter!=num_shard){
					printf("line %d : shard count %d or posting_counter %d != %d\n",i,shard_counter,posting_counter,num_shard);
					return -1;
				}
				// printf("\n");
				
				free(tokens);
			}
		}
	}
	fclose(time_file);
	
	
	/***finish process query time***/
	return 0;

}

char** str_split(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp){
        if (a_delim == *tmp){
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
	
	result = new char*[count];
    // result = (char**)malloc(sizeof(char*) * count);
	
    if (result){
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token){
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void rand_array_with_fix_sum(int* array, int length, int sum){
	// int *temp=(int*) calloc(length, sizeof(int));
	int *temp=new int[length];
	int i,j,a;
	
	for(i=0;i<length;i++){
		temp[i]=rand()%sum;
	}
	// sort temp[1-length]
	for (i = 0; i < length; ++i){
        for (j = i + 1; j < length; ++j){
            if (temp[i] > temp[j]){
                a =  temp[i];
                temp[i] = temp[j];
                temp[j] = a;
            }
        }
    }
	
	
	array[0]=temp[0];
	for(i=1;i<length-1;i++){
			array[i]=temp[i]-temp[i-1];
	}
	array[length-1]=sum-temp[length-1];
	// for(i=0;i<length;i++){
		// printf("%d\t",array[i]);
	// }
	// printf("\n");

}