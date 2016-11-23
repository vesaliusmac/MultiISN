#include "common.h"
// #include "class.h"

// extern int select_f; // selected frequency
// extern double p; // load
// extern int m; // # of cores
// extern int C_state; // which c state

// extern double Pa; //active power
// extern double S; //support circuit power
// extern double wake_up_latency;
// extern double Pc;
// extern double average_service_time;
// extern double Ta;
// extern double Ts_dvfs;

// extern double* service_length;
// extern double* service_cdf;
// extern double* arrival_length;
// extern double* arrival_cdf;
// extern int service_count,arrival_count;
// extern int exponential;

// extern int **latency_hist,**server_idle_time_hist,**server_busy_time_hist,**server_busy_P_state_time_hist;
// extern double *package_idle_time_hist;
// extern int *server_idle_counter,*server_busy_counter,*server_wakeup_counter,*server_pkts_counter;
// extern int package_idle_counter;
// extern int pkt_index;
// extern int pick;
// extern int map[];
// extern int check_package;

// extern Queue *queue;
// extern Server *server;
// extern Package package;

int handle_arrive(double cur_time,double* event);
int handle_depart(double cur_time,double* event);