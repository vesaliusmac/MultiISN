//----- Constants -------------------------------------------------------------
// #define ivy
#define PKT_limit 300000//0 // Simulation time
#define quiet 1
#define debug 0
#define package_sleep 0
#define SIM_TIME DBL_MAX
#define Queue_length 1024
#define bin_count 1000000
#define bin_width 1
// #define agg_timeout 1500000
#define agg_collect 10000
// #define ISN_timeout 150000
#define ISN_prediction_overhead 7000
// #define reQuery_threshold 15
#ifdef ivy
#define server_count 4 //no of servers
#else
#define server_count 12 //no of servers
#endif

#define ISN_event_count 2  //[ISN_receive_pkt ISN_depart_pkt] 
#define Agg_event_count 4  //[Agg_receive_pkt Agg_depart_pkt Agg_pkt_timeout Agg_collect]
#define num_ISN 16
#define latency_bound 100000
#define alpha 1.0
#define FILE_SERVICE "/home/chou/datacenter/trace/memcached_service_cdf.log"
#define FILE_ARRIVAL "/home/chou/datacenter/trace/memcached_arrival_cdf.log"
// #define FILE_SERVICE "/home/chou/datacenter/trace/search_service_cdf.log"
// #define FILE_ARRIVAL "/home/chou/datacenter/trace/search_arrival_cdf.log"