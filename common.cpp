#include "common.h"



void error( const char* format, ... ) {
    if(debug==1){
		va_list args;
		// fprintf( stderr, "Error: " );
		va_start( args, format );
		vfprintf( stderr, format, args );
		va_end( args );
		// fprintf( stderr, "\n" );
	}
}


/************************************************************************
Function to generate exponentially distributed RVs 
Input: x (mean value of distribution) 
Output: Returns with exponential RV 
*************************************************************************/
double expntl(double x){
	double z; // Uniform random number from 0 to 1

	// Pull a uniform RV (0 < z < 1)
	do{
		z = ((double) rand() / RAND_MAX);
	}
	while ((z == 0) || (z == 1));

	return(-x * log(z));
}


int rand_int(int b)
{
    //The basic function here is like (rand() % range)+ Lower_bound

        return((rand() % (b)));
    
}




int generate_iat(int count, double* cdf){
	int i;
		
	double a = (double)rand()/(double)RAND_MAX;
	for(i=0;i<count;i++){
		if(cdf[i]>a){
			return i;
		}
	}
	return count;
}

void read_dist(double* avg_service_time){
	FILE *trace_file;
	
	trace_file = fopen(FILE_SERVICE, "r");
	// printf("fopend\n");
	int line_count=0;
	double unit;
	fscanf(trace_file, "%d%lf", &line_count, &unit);
	// printf("%d\t%f\n",line_count,unit);
	service_count=line_count;
	// double* service_length;
// double* service_cdf;
	service_length = (double *)malloc(line_count*sizeof(double));
	service_cdf = (double *)malloc(line_count*sizeof(double));
	double *pdf = (double *)malloc(line_count*sizeof(double));
	double a,b;
	int drop=0;
	int read_counter=0;
	while (fscanf(trace_file, "%lf%lf", &a,&b) != EOF) {
		service_length[read_counter]=a;
		service_cdf[read_counter]=b;
		// printf("%f\t%f\n",service_length[read_counter],service_cdf[read_counter]);
		read_counter++;
		
	}
	fclose(trace_file);
	
	double average=0;
	double check=0;
	for(int i=0;i<line_count;i++){
		if(i==0) 
			pdf[i]=service_cdf[i];
		else 
			pdf[i]=service_cdf[i]-service_cdf[i-1];
		check=check+pdf[i];
		average=average+service_length[i]*pdf[i];
		// printf("%lf\n",pdf[i]);
	}
	*avg_service_time=average;
	
	
}


int read_and_scale_dist(double target_mean){
	FILE *trace_file;
	
	trace_file = fopen(FILE_ARRIVAL, "r");
	int line_count=0;
	double unit;
	fscanf(trace_file, "%d%lf", &line_count, &unit);
	// printf("%d\t%f\n",line_count,unit);
	arrival_count=line_count;
	// printf("%d\n",line_count);
	double *length = (double *)malloc(line_count*sizeof(double));
	double *cdf = (double *)malloc(line_count*sizeof(double));
	double *pdf = (double *)malloc(line_count*sizeof(double));
	double a,b;
	int drop=0;
	int read_counter=0;
	while (fscanf(trace_file, "%lf%lf", &a,&b) != EOF) {
		length[read_counter]=a;
		cdf[read_counter]=b;
		read_counter++;
	}
	fclose(trace_file);
	
	double average=0;
	double check=0;
	for(int i=0;i<line_count;i++){
		if(i==0) 
			pdf[i]=cdf[i];
		else 
			pdf[i]=cdf[i]-cdf[i-1];
		check=check+pdf[i];
		average=average+length[i]*pdf[i];
		// printf("%lf\n",pdf[i]);
	}
	// printf("check: %f\n",check);
	// printf("avg: %f\n",average);
	// printf("target: %f\tavg: %f\n",target_mean,average);
	double scaliing=target_mean/average;
	for(int i=0;i<line_count;i++){
		length[i]=length[i]*scaliing;
		// printf("%f\n",length[i]);
	}
	int max_new_bin=ceil((line_count-1)*scaliing);
	// printf("original: %f\ntarget: %f\nscaling: %f\nnew max bin: %d\n",average,target_mean,scaliing,max_new_bin);
	if(scaliing<1){
		// printf("scaling < 1, future work\n");
		return -1;
	}
	
	// do scaling and interpolate
	// double* arrival_length;
// double* arrival_cdf;
	arrival_length = (double *)malloc(max_new_bin*sizeof(double));
	double *new_pdf = (double *)malloc(max_new_bin*sizeof(double));
	arrival_cdf = (double *)malloc(max_new_bin*sizeof(double));
	arrival_length[0]=0;
	new_pdf[0]=pdf[0];
	int old_ptr=1;
	double distance_prev=0, distance_next=0;
	for(int i=1;i<max_new_bin;i++){
		arrival_length[i]=i*unit;
		distance_prev=arrival_length[i]-length[old_ptr-1];
		distance_next=length[old_ptr]-arrival_length[i];
		// printf("%d\t%f\t%f\t%f\n",i,new_length[i],length[old_ptr-1],length[old_ptr]);
		// printf("%f\t%f\n",distance_prev,pdf[old_ptr-1]);
		// printf("%f\t%f\n",distance_next,pdf[old_ptr]);
		if(old_ptr>=line_count) 
			new_pdf[i]=0;
		else
			new_pdf[i]=(pdf[old_ptr-1]*distance_next+pdf[old_ptr]*distance_prev)/(distance_prev+distance_next);
		// printf("%f\n",new_pdf[i]);
		if(arrival_length[i]+unit>length[old_ptr])
			old_ptr++;
		
	}
	int nf=0,nn=0;
	double new_average=0;
	check=0;
	for(int i=0;i<max_new_bin;i++){
		
		new_pdf[i]=new_pdf[i]/scaliing;
		
		check=check+new_pdf[i];
		new_average=new_average+new_pdf[i]*arrival_length[i];
		if(i==0) 
			arrival_cdf[i]=new_pdf[i];
		else 
			arrival_cdf[i]=arrival_cdf[i-1]+new_pdf[i];
		// printf("%f\t%f\t%f\n",new_length[i],new_pdf[i],new_cdf[i]);
		// printf("%d\t%f\n",i,cdf[i]);
		// if(new_cdf[i]>0.95 && nf < 1) {
			// printf("95th %d\n",i);
			
			// nf=i;
		// }
		// if(new_cdf[i]>0.99 && nn < 1) {
			// printf("99th %d\n",i);
			
			// nn=i;
		// }	
	}
	
	// printf("check: %f\n",check);
	// printf("avg: %f\n",new_average);
	// printf("95th: %f\n",nf*unit);
	// printf("99th: %f\n",nn*unit);
	return 0;
}