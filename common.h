#include <stdio.h> 
#include <stdlib.h> // Needed for rand() and RAND_MAX
#include <math.h> // Needed for log()
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <assert.h>
#include "parameter.h"
#include "class.h"

extern double freq[];
extern double voltage[];
extern double Pa_dyn, Pa_static,S_dyn, S_static;

extern int select_f; // selected frequency
extern int p; // QPS
extern int m; // # of cores
extern int C_state; // which c state
extern int ISN_timeout; // ISN timeout
extern int agg_timeout; // AGG timeout

extern double Pa; //active power
extern double S; //support circuit power
extern double wake_up_latency;
extern double Pc;
extern double average_service_time;
extern double Ta;
extern double Ts_dvfs;

extern double* service_length;
extern double* service_cdf;
extern double* arrival_length;
extern double* arrival_cdf;
extern int service_count,arrival_count;
extern int exponential;

extern int ***latency_hist,***server_idle_time_hist,***server_busy_time_hist,***server_busy_P_state_time_hist;
extern double **package_idle_time_hist;
extern int *Agg_latency_hist;
extern int **server_idle_counter,**server_busy_counter,**server_wakeup_counter,**server_pkts_counter;
extern int *package_idle_counter;
extern double *ser;

extern int pkt_index;
extern int pick;
extern int map[];
extern int check_package;

extern Queue *ISN_queue;
extern Queue *Agg_receive_queue;
extern Server **server;
extern Package *package;

extern char **query;
extern int **result_shard;
extern double *Agg_time;
extern int **shard_doc_count;
extern char **scores_query;
extern double **scores;
extern double **shard_scores;
extern char **time_query;
extern double **query_service_time;
extern double *query_agg_time;
extern int **query_posting_length;

extern int top_k;
extern int row;
extern int num_line;

void error( const char* format, ... );
template <typename T>
T* create_1D_array(int x, T init);
template <typename T>
T** create_2D_array(int x, int y, T init);
template <typename T>
T*** create_3D_array(int x, int y, int z, T init);

double expntl(double x);
int rand_int(int b);
int generate_iat(int count, double* cdf);
int read_and_scale_dist(double target_mean);
void read_dist(double* avg_service_time);