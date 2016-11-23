#include "GGk_default.h"
#include "template_impl.tpp"

// ivybridge
#ifdef ivy
double freq[13]={3.4,3.3,3.2,3,2.9,2.8,2.6,2.4,2.2,2.1,2,1.7,1.6};
// double freq[16]={3.4,3.1875,2.975,2.7625,2.55,2.3375,2.125,1.9125,1.7,1.4875,1.275,1.0625,0.85,0.6375,0.425,0.2125}; //dfs
double voltage[13]={1.070800781,1.045776367,1.025756836,0.990722656,0.975708008,0.955688477,0.955688477,0.955688477,0.950683594,0.950683594,0.950683594,0.945678711,0.945678711};
// Pa = 0.708255*v+1.474418*v^2*f
double Pa_dyn=1.474418, Pa_static=0.708255;
double S_dyn=1.0747, S_static=0.9002;

#else
// xeonphi
double freq[15]={2.7,2.5,2.4,2.3,2.2,2.1,2,1.9,1.8,1.7,1.6,1.5,1.4,1.3,1.2};
double voltage[15]={0.990722656,0.970703125,0.960693359,0.955688477,0.945678711,0.940673828,0.935668945,0.92565918,0.920654297,0.910644531,0.900634766,0.895629883,0.890625,0.885620117,0.875610352};
// Pa = 0.2261*v+1.2208*v^2*f
double Pa_dyn=1.2208, Pa_static=0.2261;
double S_dyn=4.8233, S_static=2.6099;


#endif
/*input arg*/
int select_f; // selected frequency
double p; // load
int m; // # of cores
int C_state; // which c state

/*system config*/
double Pa; //active power
double S; //support circuit power
double wake_up_latency;
double Pc;
double average_service_time;
double Ta;
double Ts_dvfs;
Pkt NULL_PKT;

/*read trace*/
double* service_length;
double* service_cdf;
double* arrival_length;
double* arrival_cdf;
int service_count,arrival_count;
int exponential=0;

int **latency_hist,**server_idle_time_hist,**server_busy_time_hist,**server_busy_P_state_time_hist;
double *package_idle_time_hist;
int *server_idle_counter,*server_busy_counter,*server_wakeup_counter,*server_pkts_counter;
int package_idle_counter=0;
int pkt_index=0;

int pick;
int map[server_count]={0};
int check_package=0;

Queue *queue;
Server *server;
Package package;


/********************** Main program******************************/
int main(int argc, char **argv){
	srand(time(NULL));
	/*process input arguments*/
	if(argc!=5) {
		printf("use: [freq] [load] [core count] [C-state]\n");
		return 0;
	}
	select_f=atoi(argv[1]); // selected frequency
	p =atof(argv[2]); // load
	m=atoi(argv[3]); // # of cores
	if(m>server_count) m=server_count;
	C_state = atoi(argv[4]); // which c state
	
	/*configure the power and wakeup metrics*/
	Pa = Pa_static*voltage[select_f]+Pa_dyn*voltage[select_f]*voltage[select_f]*freq[select_f]; 
	S = S_static*voltage[select_f]+S_dyn*voltage[select_f]*voltage[select_f]*freq[select_f]+5; //support circuit power
	
	switch(C_state){
		case 1:
			wake_up_latency=1;
			Pc=0.5*Pa;
			break;
		case 3:
			wake_up_latency=59;
			Pc=0.708255*voltage[select_f];
			break;
		case 6:
			wake_up_latency=89;
			Pc=0;
			break;
		default:
			wake_up_latency=1;
			Pc=0.5*Pa;
			C_state=1;
			printf("input C-state %d does not exist!! use C1 state by default\n");
			break;
	}
	
	
	int i,j,kk,jj;
	
	/*create arrays for performance statistics*/
	
	server_idle_counter = create_1D_array<int>(m,0);
	server_busy_counter = create_1D_array<int>(m,0);
	server_wakeup_counter = create_1D_array<int>(m,0);
	server_pkts_counter = create_1D_array<int>(m,0);
		
	latency_hist=create_2D_array<int>(m,bin_count,0); // per-core latency
	server_idle_time_hist=create_2D_array<int>(m,bin_count,0); // per-core idle time
	server_busy_time_hist=create_2D_array<int>(m,bin_count,0); // per-core busy time
	package_idle_time_hist = create_1D_array<double>(bin_count,0); // processor idle time
	
	/*create servers*/
	server = new Server[m];
	/*create a global queue*/
	queue = new Queue;
	
	/*configure simulator*/
	static double time = 0.0; // Simulation time stamp
	double event[event_count]={0,SIM_TIME};
	int next_event; // what is next event
	double next_event_time; // what is the time of next event
	/*read trace*/
	// double average_service_time;
	read_dist(&average_service_time);
	Ta=average_service_time/(m*p);
	if(read_and_scale_dist(Ta)<0) {
		exponential=1; // can not scale, use exponential arrival dist
	}
	
	double Ts_dvfs=average_service_time*freq[0]/freq[select_f]*alpha+average_service_time*(1-alpha);
	if(Ts_dvfs/Ta/m>1){
		printf("selected frequency is too low\n");
		return 0;
	}
	
	
	/*print system config*/
	if(!quiet){
		printf("\n********System Config*******\n\n");
		printf("system load %f\n",Ts_dvfs/Ta/m);
		printf("# of cores %d\n",m);
		printf("selected frequency/voltage %.3f/%.3f\n",freq[select_f],voltage[select_f]);
		printf("mean service time %.3f\nmean arrival time %.3f\n",Ts_dvfs*1000000,Ta*1000000);
		printf("Pa %.3f, S %.3f\n",Pa,S);
	}
	
	
	
	
	
	/**********************/
	/*Main simulation loop*/
	/**********************/
	
	while (pkt_index < PKT_limit){
		// find next event
		next_event_time=SIM_TIME;
		for(i=0;i<event_count;i++){
			if(event[i]<next_event_time){
				next_event_time=event[i];
				next_event=i;
			}
		}
		
		// check which event
		switch(next_event){
			case 0: // *** Event pkt arrive at ISN
				if(next_event_time!=event[0]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time=event[0]; // advance the time
				
				// handle pkt arrival
				if(handle_arrive(time,event)<0){
					printf("handle arrive error\n");
					return 0;
				}
				break;
			case 1: // *** Event pkt departure
				if(next_event_time!=event[1]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time = event[1]; // advance the time
				
				// handle pkt departure
				if(handle_depart(time,event)<0){
					printf("handle depart error\n");
					return 0;
				}
				break;
			case 2: // *** Event pkt arrive at aggregator
			default:
				printf("event error\n");
				return 0;
		}
		
	}
	
	
	
	// wrap up the busy/idle period counter
	int which_bin;
	for (i=0;i<m;i++){
		if(server[i].state==0){
			which_bin=floor((time-server[i].time_arrived)/bin_width);
			if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
			if (which_bin > bin_count-1) which_bin=bin_count-1;
			server_idle_time_hist[i][which_bin]++;
			server_idle_counter[i]++;
			
		}else{
			which_bin=floor((time-server[i].time_arrived)/bin_width);
			if(which_bin<0-wake_up_latency) {printf("latency error %d\n",__LINE__); return 0;}
			if (which_bin > bin_count-1) which_bin=bin_count-1;
			server_busy_time_hist[i][which_bin]++;
			server_busy_counter[i]++;
			
		}
		
	}
	if(package_sleep != 0){
		if(package.time_arrived>0){
			package.time_finished=time;
			which_bin=floor((package.time_finished-package.time_arrived)/bin_width);
			if (which_bin > bin_count-1) which_bin=bin_count-1;
			package_idle_time_hist[which_bin]++;
			package.time_arrived=-1;
			package_idle_counter++;
		}
	}
	
	/*******************************/
	/*Main simulation loop finished*/
	/*******************************/
	
	/*create arrays for results computation*/
	double *per_core_busy = create_1D_array<double>(m,0); 
	double *per_core_idle = create_1D_array<double>(m,0); 
	double *per_core_wakeup = create_1D_array<double>(m,0); 
	double *per_core_total_time = create_1D_array<double>(m,0); 
	
	double *per_core_busy_ratio = create_1D_array<double>(m,0); 
	double *per_core_idle_ratio = create_1D_array<double>(m,0); 
	double *per_core_wakeup_ratio = create_1D_array<double>(m,0); 
	
	double overall_total_time=0,overall_busy=0,overall_idle=0,overall_wakeup=0;
	
	for(j=0;j<m;j++){
		for(i=0;i<bin_count;i++){
			per_core_idle[j]+=server_idle_time_hist[j][i]*i;
			per_core_busy[j]+=server_busy_time_hist[j][i]*i;
		}
		per_core_wakeup[j]=server_wakeup_counter[j]*wake_up_latency;
	}
	
	
	for(j=0;j<m;j++){
		per_core_total_time[j]=per_core_busy[j]+per_core_idle[j]+per_core_wakeup[j];
		per_core_busy_ratio[j]=per_core_busy[j]/per_core_total_time[j];
		per_core_idle_ratio[j]=per_core_idle[j]/per_core_total_time[j];
		per_core_wakeup_ratio[j]=per_core_wakeup[j]/per_core_total_time[j];
		
		overall_total_time+=per_core_total_time[j];
		overall_busy+=per_core_busy[j];
		overall_idle+=per_core_idle[j];
		overall_wakeup+=per_core_wakeup[j];
	}
	
	double overall_busy_ratio=overall_busy/overall_total_time;
	double overall_idle_ratio=overall_idle/overall_total_time;
	double overall_wakeup_ratio=overall_wakeup/overall_total_time;
	
	int pkt_processed=0,pkt_inserver=0;
	for(j=0;j<m;j++){
		pkt_processed+=server_pkts_counter[j];
		pkt_inserver+=server[j].state;
	}
	double overall_package_idle=0;
	
	if(package_sleep!=0){
		for(j=0;j<bin_count;j++){
			overall_package_idle=overall_package_idle+package_idle_time_hist[j]*j;
		}
	}
	
	if(!quiet){
		printf("\n*********Result*********\n\n");
		printf("total pkts arrived %d\n",pkt_index);
		printf("total pkts processed %d\n",pkt_processed);
		printf("total pkts in queue %d\n",queue->getQlength());
		printf("total pkts in servers %d\n",pkt_inserver);
		if(package_sleep!=0) printf("package idle ratio %f\n",overall_package_idle/time);
	}
	
	// latency dist
	int nfp=0,nnp=0;
	int *all_latency = create_1D_array<int>(bin_count,0); 
	
	int total_pkts=0;
	for(i=0;i<bin_count;i++){
		for(j=0;j<m;j++){
			all_latency[i]+=latency_hist[j][i];
		}
		total_pkts+=all_latency[i];
	}
	int temp_count=0;
	for(i=0;i<bin_count;i++){
		temp_count+=all_latency[i];
		if(temp_count>total_pkts*0.95 && nfp==0){
			nfp=i*bin_width;
		}
		if(temp_count>total_pkts*0.99 && nnp==0){
			nnp=i*bin_width;
		}
	}
	double *per_core_latency = create_1D_array<double>(m,0); 
	
	double overall_latency=0;
	for(j=0;j<m;j++){
		for(i=0;i<bin_count;i++){
			per_core_latency[j]=per_core_latency[j]+i*bin_width*latency_hist[j][i];
		}
		overall_latency+=per_core_latency[j];
		per_core_latency[j]=per_core_latency[j]/server_pkts_counter[j];
		
	}
	
	/*print results*/
	if(!quiet){
		printf("average latency %f\n",overall_latency/pkt_processed);
		printf("average busy ratio %f\n",overall_busy_ratio);
		printf("average idle ratio %f\n",overall_idle_ratio);
		printf("average wakeup ratio %f\n",overall_wakeup_ratio);
		printf("average power %f\n",(Pa*(overall_busy_ratio+overall_wakeup_ratio)+Pc*overall_idle_ratio)*m);
	}
	if(0){
		printf("\n************per server***********\n\n");
		printf("busy period count\t");
		for(j=0;j<m;j++){
			printf("%d\t",server_busy_counter[j]);
		}
		printf("\n");
		printf("wake up count\t");
		for(j=0;j<m;j++){
			printf("%d\t",server_wakeup_counter[j]);
		}
		printf("\n");
		for(j=0;j<m;j++){
			printf("server %d: pkts : %d\t busy ratio: %f\t idle ratio: %f\t wakeup ratio: %f\tavg latency : %f\n",j,server_pkts_counter[j],per_core_busy_ratio[j],per_core_idle_ratio[j],per_core_wakeup_ratio[j],per_core_latency[j]);
			
		}
	}
	
	
	printf("%d\t%.2f\t%f\t%f\t%d\t%d\t%d\tC%d\n",m,p,(Pa*(overall_busy_ratio+overall_wakeup_ratio)+Pc*overall_idle_ratio)*m,overall_latency/pkt_processed,nfp,nnp,0,C_state);
	
	return 0;

}




