#include <stdio.h> 
#include <stdlib.h> // Needed for rand() and RAND_MAX
#include <math.h> // Needed for log()
#include <time.h>
#include <stdarg.h>
#include <float.h>


//----- Constants -------------------------------------------------------------
// #define ivy
#define PKT_limit 3000000 // Simulation time
#define quiet 0
#define debug 0
#define package_sleep 0
#define SIM_TIME DBL_MAX
#define Queue_length 1024
#define bin_count 10000
#define bin_width 1
#define agg_timeout 100
#include "class.h"

#ifdef ivy
#define server_count 4 //no of servers
#else
#define server_count 12 //no of servers
#endif

#define event_count 5  //[ISN_receive_pkt ISN_depart_pkt Agg_receive_pkt Agg_depart_pkt Agg_pkt_timeout]
#define num_ISN 1
#define latency_bound 100000
#define alpha 1.0
#define FILE_SERVICE "/home/chou/datacenter/trace/memcached_service_cdf.log"
#define FILE_ARRIVAL "/home/chou/datacenter/trace/memcached_arrival_cdf.log"
// #define FILE_SERVICE "/home/chou/datacenter/trace/search_service_cdf.log"
// #define FILE_ARRIVAL "/home/chou/datacenter/trace/search_arrival_cdf.log"


extern double freq[];
extern double voltage[];
extern double Pa_dyn, Pa_static,S_dyn, S_static;

extern int select_f; // selected frequency
extern double p; // load
extern int m; // # of cores
extern int C_state; // which c state

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

extern int **latency_hist,**server_idle_time_hist,**server_busy_time_hist,**server_busy_P_state_time_hist;
extern double *package_idle_time_hist;
extern int *Agg_latency_hist;
extern int *server_idle_counter,*server_busy_counter,*server_wakeup_counter,*server_pkts_counter;
extern int package_idle_counter;
extern int pkt_index;
extern int pick;
extern int map[];
extern int check_package;

extern Queue *ISN_queue;
extern Queue *Agg_receive_queue;
extern Server *server;
extern Package package;


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