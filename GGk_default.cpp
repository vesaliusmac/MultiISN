#include "GGk_default.h"
#include "template_impl.tpp"
#include "read_trace.h"

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
int p; // QPS
int m; // # of cores
int C_state; // which c state
int ISN_timeout; // ISN timeout
int agg_timeout; // AGG timeout
int reQuery_threshold; // reQuery threshold
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

int ***latency_hist,***server_idle_time_hist,***server_busy_time_hist,***server_busy_P_state_time_hist;
double **package_idle_time_hist;
int *Agg_latency_hist;
int *Agg_quality_hist;
int *Agg_drop_hist;
int **server_idle_counter,**server_busy_counter,**server_wakeup_counter,**server_pkts_counter;
int *package_idle_counter;
int pkt_index=0;

double **ISN_event;
double *Agg_event;

int pick;
int map[server_count]={0};
int check_package=0;

Queue *ISN_queue;
Queue *Agg_receive_queue;
Server **server;
Package *package;
Agg_wait_list *wait_list;

int top_k;

/********************** Main program******************************/
int main(int argc, char **argv){
	srand(time(NULL));
	/*process input arguments*/
	if(argc!=6) {
		printf("use: [QPS] [top_k] [AGG timeout] [ISN timeout] [requery threshold]\n");
		return 0;
	}
	// select_f=atoi(argv[1]); // selected frequency
	select_f=0;
	p = atoi(argv[1]); // QPS
	
	// m=atoi(argv[3]); // # of cores
	m=server_count;
	// if(m>server_count) m=server_count;
	// C_state = atoi(argv[4]); // which c state
	C_state=6;
	top_k=atoi(argv[2]); // top k
	agg_timeout = atoi(argv[3]); // AGG timeout
	ISN_timeout = atoi(argv[4]); // ISN timeout
	reQuery_threshold = atoi(argv[5]); // requery threshold
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
	
	
	int i,j,k,kk,jj;
	
	/*create arrays for performance statistics*/
	
	server_idle_counter = create_2D_array<int>(num_ISN,m,0);
	server_busy_counter = create_2D_array<int>(num_ISN,m,0);
	server_wakeup_counter = create_2D_array<int>(num_ISN,m,0);
	server_pkts_counter = create_2D_array<int>(num_ISN,m,0);
	package_idle_counter = create_1D_array<int>(num_ISN,0);
	
	latency_hist = create_3D_array<int>(num_ISN,m,bin_count,0); // per-core latency
	server_idle_time_hist = create_3D_array<int>(num_ISN,m,bin_count,0); // per-core idle time
	server_busy_time_hist = create_3D_array<int>(num_ISN,m,bin_count,0); // per-core busy time
	package_idle_time_hist = create_2D_array<double>(num_ISN,bin_count,0); // processor idle time
	Agg_latency_hist = create_1D_array<int>(bin_count,0); // Aggregator pkt latency
	Agg_quality_hist = create_1D_array<int>(101,0); // Aggregator quality (0..1..100)
	Agg_drop_hist = create_1D_array<int>(num_ISN+1,0); // Aggregator quality (0..1..100)
	/*create servers*/
	server = new Server*[num_ISN];
	for(i=0;i<num_ISN;i++){
		server[i] = new Server[m];
	}
	/*create packages*/
	package = new Package[num_ISN];
	
	/*create ISN queue and Aggregator recv queue*/
	ISN_queue = new Queue[num_ISN];
	Agg_receive_queue = new Queue;
	
	/*create aggregator wait list*/
	wait_list= new Agg_wait_list;
	
	/*configure simulator*/
	static double time = 0.0; // Simulation time stamp
	/*create event time keeper*/
	ISN_event = create_2D_array<double>(num_ISN,ISN_event_count,SIM_TIME);
	Agg_event = create_1D_array<double>(Agg_event_count,SIM_TIME);
	Agg_event[0]=0;
	// double event[event_count]={SIM_TIME,SIM_TIME,0,SIM_TIME,SIM_TIME}; // init event
	int next_event; // what is next event
	double next_event_time; // what is the time of next event
	int next_event_ISN; // which ISN is next event
	
	/*read trace*/
	if(read_trace()<0){
		printf("read trace error\n");
		return 0;
	}
	// for(i=0;i<10;i++){
		// for(j=0;j<num_shard;j++){
			// printf("%f\t",query_service_time[i][j]);
		// }
		// printf("\n");
		// for(j=0;j<num_shard;j++){
			// printf("%d\t",query_posting_length[i][j]);
		// }
		// printf("\n");
	
	// }
	Ta=1.0/p*1000000;
	exponential=1;
	// read_dist(&average_service_time);
	// Ta=average_service_time/(m*p);
	// if(read_and_scale_dist(Ta)<0) {
		// exponential=1; // can not scale, use exponential arrival dist
	// }
	
	// double Ts_dvfs=average_service_time*freq[0]/freq[select_f]*alpha+average_service_time*(1-alpha);
	// if(Ts_dvfs/Ta/m>1){
		// printf("selected frequency is too low\n");
		// return 0;
	// }
	
	
	/*print system config*/
	if(!quiet){
		printf("\n********System Config*******\n\n");
		printf("# of ISN %d\n",num_ISN);
		printf("top_k is %d\n",top_k);
		printf("Aggregator timeout %d\n",agg_timeout);
		// printf("system load %f\n",Ts_dvfs/Ta/m);
		printf("# of cores %d\n",m);
		printf("selected frequency/voltage %.3f/%.3f\n",freq[select_f],voltage[select_f]);
		// printf("mean service time %.3f\nmean arrival time %.3f\n",Ts_dvfs*1000000,Ta*1000000);
		printf("Pa %.3f, S %.3f\n",Pa,S);
	}
	
	
	
	
	static Pkt incoming_pkt,fetched_pkt;
	double pkt_service_time;
	double inter_arrival;
	static int which_bin;
	static int which_pkt;
	int Agg_pkt_processed=0;
	int time_out_counter=0;
	double Golden_score=0;
	double Agg_timeout_score=0;
	int dropped_ISN=0;
	int *drop_array;
	int wait_ISN=0;
	/**********************/
	/*Main simulation loop*/
	/**********************/
	int progress=-1;
	while (pkt_index < PKT_limit){
		// print progress
		if(pkt_index%((int)(PKT_limit/10))==0){
			if(pkt_index/((int)(PKT_limit/10))!=progress){
				if(!quiet) fprintf(stderr,"%d",pkt_index/((int)(PKT_limit/10)));
				progress=pkt_index/((int)(PKT_limit/10));
			}
		}
		
		// find next event
		next_event_time=SIM_TIME;
		for(i=0;i<num_ISN;i++){
			for(j=0;j<ISN_event_count;j++){
				if(ISN_event[i][j]<next_event_time){
					next_event_time=ISN_event[i][j];
					next_event=j;
					next_event_ISN=i;
				}
			}
		}
		for(i=0;i<Agg_event_count;i++){
			if(Agg_event[i]<next_event_time){
				next_event_time=Agg_event[i];
				next_event=ISN_event_count+i;
				next_event_ISN=-1;
			}
		}
		error("%f\tevent time\t",time);
		for(i=0;i<num_ISN;i++){
			for(j=0;j<ISN_event_count;j++){
				if(ISN_event[i][j]<SIM_TIME)
					error("%f\t",ISN_event[i][j]);
				else
					error("-1\t");
			}
		}
		for(i=0;i<Agg_event_count;i++){
			if(Agg_event[i]<SIM_TIME)
				error("%f\t",Agg_event[i]);
			else
				error("-1\t");
		}
		error("\n");
		
		// check which event
		switch(next_event){
			case 0: // *** Event pkt arrive at ISN
				if(next_event_ISN<0){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=ISN_event[next_event_ISN][0]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time=next_event_time; // advance the time
				
				// handle pkt arrival
				if(handle_arrive(next_event_ISN,time,ISN_event[next_event_ISN])<0){
					printf("handle arrive error\n");
					return 0;
				}
				break;
			case 1: // *** Event pkt departure
				if(next_event_ISN<0){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=ISN_event[next_event_ISN][1]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time = next_event_time; // advance the time
				
				// handle pkt departure
				if(handle_depart(next_event_ISN,time,ISN_event[next_event_ISN],Agg_event)<0){
					printf("handle depart error\n");
					return 0;
				}
				break;
			case 2: // *** Event pkt arrive at aggregator
				if(next_event_ISN!=-1){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=Agg_event[0]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time = next_event_time; // advance the time
				
				error("%f\tpkt %d arrived at Aggregator\n",time,pkt_index);
				
				// add pkt into agg wait list
				if(wait_list->add(pkt_index,time)<0){
					printf("add aggregator wait list error\n");
					return 0;
				}
				
				// determine the arriving pkt service time
				// pkt_service_time=service_length[generate_iat(service_count,service_cdf)]*1000000;
				
				//fill up the pkt info
				incoming_pkt.index=pkt_index;
				incoming_pkt.time_arrived=time;
				// incoming_pkt.service_time=pkt_service_time*freq[0]/freq[select_f]*alpha+pkt_service_time*(1-alpha);
				incoming_pkt.time_finished=-1;
				incoming_pkt.disable_drop=0;
				
				// send pkt and insert it into ISN's queue	
				for(i=0;i<num_ISN;i++){
					incoming_pkt.which_ISN=i;
					ISN_queue[i].enQ(incoming_pkt);
				}
				// update pkt index
				pkt_index++;
				
				// determine the next pkt arrival time
				if(exponential==0)
					inter_arrival=arrival_length[generate_iat(arrival_count,arrival_cdf)]*1000000;
				else
					inter_arrival=expntl(Ta);
					
				// update event 
				for(i=0;i<num_ISN;i++){
					ISN_event[i][0] = time;
				}
				Agg_event[0] = time + inter_arrival;
				Agg_event[2] = wait_list->getTime(wait_list->find_next_timeout());
				Agg_event[3] = wait_list->getCollect(wait_list->find_next_collect());
				break;
			case 3: // *** Event aggregator receive response from ISN
				if(next_event_ISN!=-1){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=Agg_event[1]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time = next_event_time; // advance the time
				// printf("%d\n",Agg_receive_queue->getQlength());
				while(Agg_receive_queue->getQlength()>0){ // process the response from ISN
					fetched_pkt=Agg_receive_queue->deQ();
					if(fetched_pkt.index==-1){
						printf("deQ error\n");
						return -1;
					}
					error("%f\tAggregator receives response %d from ISN\n",time,fetched_pkt.index);
					if(fetched_pkt.reQuery_response==1)
						error("%f\treceive reQuery response for pkt %d\n",time,fetched_pkt.index);
					wait_list->insert(fetched_pkt);
					// if(wait_list->insert(fetched_pkt.index)<0){
						// printf("insert aggregator wait list error\n");
						// return 0;
					// }
					dropped_ISN=wait_list->getDropCounter(fetched_pkt.index);
					if(dropped_ISN<0){
						printf("dropped_ISN error\n");
						return 0;
					}
					if(dropped_ISN>reQuery_threshold)
						wait_ISN=num_ISN+dropped_ISN;
					else
						wait_ISN=num_ISN;
					if(wait_list->getCounter(fetched_pkt.index)==wait_ISN){
						error("%f\tAggregator reply response %d back to user\n",time,fetched_pkt.index);
						Agg_pkt_processed++;
						
						which_bin=floor((fetched_pkt.time_finished-fetched_pkt.time_arrived)/bin_width);
						if(which_bin<0) {printf("latency error %d\n",__LINE__); printf("%f\t%f\n",fetched_pkt.time_finished,fetched_pkt.time_arrived); return 0;}
						if (which_bin > bin_count-1) which_bin=bin_count-1;
						Agg_latency_hist[which_bin]++;
						// Agg_quality_hist[100]++;
						// log Aggregator quality
						Golden_score=0;
						for(i=0;i<top_k;i++){ // compute the Golden score
							Golden_score+=scores[fetched_pkt.index%num_line][i];
						}
						// compute the received scores
						Agg_timeout_score=0;
						wait_list->sort_score(fetched_pkt.index);
						
						// compute the timeout score
						Agg_timeout_score=wait_list->getScore(fetched_pkt.index,top_k);
						error("%f\tpkt %d\tgolden %f\tagg %f\n",time,fetched_pkt.index%num_line,Golden_score,Agg_timeout_score);
						if(Agg_timeout_score<0){
							printf("Agg quality calculation error\n");
							return 0;
						}
						
						if(Golden_score==0)
							which_bin=100;
						else
							which_bin=floor(100*Agg_timeout_score/Golden_score);
						
						Agg_quality_hist[which_bin]++;
						// dropped_ISN=wait_list->getDropCounter(fetched_pkt.index);
						if(dropped_ISN>reQuery_threshold)
							Agg_drop_hist[0]++;
						else
							Agg_drop_hist[dropped_ISN]++;
						wait_list->remove(fetched_pkt.index);
					}
				}
				
				Agg_event[1] = SIM_TIME;
				Agg_event[2] = wait_list->getTime(wait_list->find_next_timeout());
				Agg_event[3] = wait_list->getCollect(wait_list->find_next_collect());
				break;
			case 4: // *** Event aggregator pkt timeout
				if(next_event_ISN!=-1){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=Agg_event[2]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time=next_event_time; // advance the time
				
				// find which member times out
				which_pkt=wait_list->find_next_timeout();
				if(time!=wait_list->getTime(which_pkt)){
					printf("find the member who times out error\n");
					return 0;
				}
				
				error("%f\tAggregator pkt %d timeout\n",time,which_pkt);
				time_out_counter++;
				Agg_pkt_processed++;
				// log Aggregator latency
				which_bin=floor((time-wait_list->getArriveTime(which_pkt))/bin_width);
				if(which_bin<0) {printf("latency error %d\n",__LINE__); printf("%f\t%f\n",time,wait_list->getArriveTime(which_pkt)); return 0;}
				if (which_bin > bin_count-1) which_bin=bin_count-1;
				Agg_latency_hist[which_bin]++;
				// log Aggregator quality
				Golden_score=0;
				for(i=0;i<top_k;i++){ // compute the Golden score
					Golden_score+=scores[which_pkt%num_line][i];
				}
				// compute the received scores
				Agg_timeout_score=0;
				wait_list->sort_score(which_pkt);
				
				// compute the timeout score
				Agg_timeout_score=wait_list->getScore(which_pkt,top_k);
				error("%f\tpkt %d\tgolden %f\tagg %f\n",time,which_pkt%num_line,Golden_score,Agg_timeout_score);
				if(Agg_timeout_score<0){
					printf("Agg quality calculation error\n");
					return 0;
				}
				
				which_bin=floor(100*Agg_timeout_score/Golden_score);
				Agg_quality_hist[which_bin]++;
				
				wait_list->remove(which_pkt);
				
				Agg_event[2] = wait_list->getTime(wait_list->find_next_timeout());
				Agg_event[3] = wait_list->getCollect(wait_list->find_next_collect());
				break;
			case 5: // *** Event aggregator check ISN drop
				if(next_event_ISN!=-1){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				if(next_event_time!=Agg_event[3]){ // sanity check
					printf("line %d: event sanity check failed\n",__LINE__);
					return 0;
				}
				time=next_event_time; // advance the time
				
				which_pkt=wait_list->find_next_collect();
				
				while(Agg_receive_queue->getQlength()>0){ // process the response from ISN
					printf("%f\tthere is pkt in the queue!?\n",time);
					fetched_pkt=Agg_receive_queue->deQ();
					if(fetched_pkt.index==-1){
						printf("deQ error\n");
						return -1;
					}
					error("%f\tAggregator receives response %d from ISN\n",time,fetched_pkt.index);
					if(fetched_pkt.index!=which_pkt){
						printf("receive different reply than expected one\n");
						return -1;
					
					}
					if(fetched_pkt.reply_drop!=1){
						printf("should receive a dropped reply, but not\n");
						return -1;
					}
					wait_list->insert(fetched_pkt);
					
				}
				
				
				dropped_ISN=wait_list->getDropCounter(which_pkt);
				drop_array=wait_list->getDropArray(which_pkt);
				if(dropped_ISN>reQuery_threshold){
					//fill up the re-query pkt info
					incoming_pkt.index=which_pkt;
					incoming_pkt.time_arrived=time;
					// incoming_pkt.service_time=pkt_service_time*freq[0]/freq[select_f]*alpha+pkt_service_time*(1-alpha);
					incoming_pkt.time_finished=-1;
					incoming_pkt.disable_drop=1;
					
					// send pkt and insert it into ISN's queue	
					for(i=0;i<dropped_ISN;i++){
						incoming_pkt.which_ISN=drop_array[i];
						ISN_queue[drop_array[i]].enQ(incoming_pkt);
					}
					error("%f\tAggregator reQuery pkt %d\n",time,which_pkt);
					// printf("%d\t",dropped_ISN);
					// for(i=0;i<num_ISN;i++){
						// if(drop_array[i]>=0)
							// printf("%d\t",drop_array[i]);
					// }
					// printf("\n");
				}
				error("%f\tAggregator check %d ISNs has dropped pkt %d after %f\n",time,dropped_ISN,which_pkt,time-wait_list->getArriveTime(which_pkt));
				
				wait_list->resetCollect(which_pkt);
				if(dropped_ISN>reQuery_threshold){
					for(i=0;i<dropped_ISN;i++){
						ISN_event[drop_array[i]][0] = time;
					}
				}
				Agg_event[3] = wait_list->getCollect(wait_list->find_next_collect());
				break;
			default:
				
				printf("event error\n");
				return 0;
		}
		
	}
	
	
	
	// wrap up the busy/idle period counter
	for(i=0;i<num_ISN;i++){
		for (j=0;j<m;j++){
			if(server[i][j].state==0){
				which_bin=floor((time-server[i][j].time_arrived)/bin_width);
				if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
				if (which_bin > bin_count-1) which_bin=bin_count-1;
				server_idle_time_hist[i][j][which_bin]++;
				server_idle_counter[i][j]++;
				
			}else{
				which_bin=floor((time-server[i][j].time_arrived)/bin_width);
				if(which_bin<0-wake_up_latency) {printf("latency error %d\n",__LINE__); return 0;}
				if (which_bin > bin_count-1) which_bin=bin_count-1;
				server_busy_time_hist[i][j][which_bin]++;
				server_busy_counter[i][j]++;
				
			}
			
		}
	}
	if(package_sleep != 0){
		for(i=0;i<num_ISN;i++){
			if(package[i].time_arrived>0){
				package[i].time_finished=time;
				which_bin=floor((package[i].time_finished-package[i].time_arrived)/bin_width);
				if (which_bin > bin_count-1) which_bin=bin_count-1;
				package_idle_time_hist[i][which_bin]++;
				package[i].time_arrived=-1;
				package_idle_counter[i]++;
			}
		}
	}
	
	/*******************************/
	/*Main simulation loop finished*/
	/*******************************/
	
	/*create arrays for results computation*/
	double **per_core_busy = create_2D_array<double>(num_ISN,m,0); 
	double **per_core_idle = create_2D_array<double>(num_ISN,m,0); 
	double **per_core_wakeup = create_2D_array<double>(num_ISN,m,0); 
	double **per_core_total_time = create_2D_array<double>(num_ISN,m,0); 
	
	double **per_core_busy_ratio = create_2D_array<double>(num_ISN,m,0); 
	double **per_core_idle_ratio = create_2D_array<double>(num_ISN,m,0); 
	double **per_core_wakeup_ratio = create_2D_array<double>(num_ISN,m,0); 
	
	double *overall_total_time = create_1D_array<double>(num_ISN,0); 
	double *overall_busy = create_1D_array<double>(num_ISN,0); 
	double *overall_idle = create_1D_array<double>(num_ISN,0); 
	double *overall_wakeup = create_1D_array<double>(num_ISN,0); 
	
	for(k=0;k<num_ISN;k++){
		for(j=0;j<m;j++){
			for(i=0;i<bin_count;i++){
				per_core_idle[k][j]+=server_idle_time_hist[k][j][i]*i;
				per_core_busy[k][j]+=server_busy_time_hist[k][j][i]*i;
			}
			per_core_wakeup[k][j]=server_wakeup_counter[k][j]*wake_up_latency;
		}
	}
	
	for(i=0;i<num_ISN;i++){
		for(j=0;j<m;j++){
			per_core_total_time[i][j]=per_core_busy[i][j]+per_core_idle[i][j]+per_core_wakeup[i][j];
			per_core_busy_ratio[i][j]=per_core_busy[i][j]/per_core_total_time[i][j];
			per_core_idle_ratio[i][j]=per_core_idle[i][j]/per_core_total_time[i][j];
			per_core_wakeup_ratio[i][j]=per_core_wakeup[i][j]/per_core_total_time[i][j];
			
			overall_total_time[i]+=per_core_total_time[i][j];
			overall_busy[i]+=per_core_busy[i][j];
			overall_idle[i]+=per_core_idle[i][j];
			overall_wakeup[i]+=per_core_wakeup[i][j];
		}
	}
	double *overall_busy_ratio = create_1D_array<double>(num_ISN,0); 
	double *overall_idle_ratio = create_1D_array<double>(num_ISN,0); 
	double *overall_wakeup_ratio = create_1D_array<double>(num_ISN,0); 
	for(i=0;i<num_ISN;i++){
		overall_busy_ratio[i]=overall_busy[i]/overall_total_time[i];
		overall_idle_ratio[i]=overall_idle[i]/overall_total_time[i];
		overall_wakeup_ratio[i]=overall_wakeup[i]/overall_total_time[i];
	}
	// double overall_busy_ratio=overall_busy/overall_total_time;
	// double overall_idle_ratio=overall_idle/overall_total_time;
	// double overall_wakeup_ratio=overall_wakeup/overall_total_time;
	
	// int pkt_processed=0,pkt_inserver=0;
	int *pkt_processed = create_1D_array<int>(num_ISN,0); 
	int *pkt_inserver = create_1D_array<int>(num_ISN,0); 
	for(i=0;i<num_ISN;i++){
		for(j=0;j<m;j++){
			pkt_processed[i]+=server_pkts_counter[i][j];
			pkt_inserver[i]+=server[i][j].state;
		}
	}
	// double overall_package_idle=0;
	double *overall_package_idle = create_1D_array<double>(num_ISN,0); 
	if(package_sleep!=0){
		for(i=0;i<num_ISN;i++){
			for(j=0;j<bin_count;j++){
				overall_package_idle[i]=overall_package_idle[i]+package_idle_time_hist[i][j]*j;
			}
		}
	}
	
	if(!quiet){
		printf("\n*********Result*********\n\n");
		for(i=0;i<num_ISN;i++){
			printf("ISN %d\ttotal pkts arrived %d\n",i,pkt_index);
			printf("ISN %d\ttotal pkts processed %d\n",i,pkt_processed[i]);
			printf("ISN %d\ttotal pkts in queue %d\n",i,ISN_queue[i].getQlength());
			printf("ISN %d\ttotal pkts in servers %d\n",i,pkt_inserver[i]);
			if(package_sleep!=0) printf("ISN %d\tpackage idle ratio %f\n",i,overall_package_idle[i]/time);
		}
		printf("total pkts timeout ratio at aggregator %f\n",time_out_counter*1.0/Agg_pkt_processed);
	}
	
	// latency dist
	// int nfp=0,nnp=0;
	int *nfp = create_1D_array<int>(num_ISN,0); 
	int *nnp = create_1D_array<int>(num_ISN,0); 
	
	int **all_latency = create_2D_array<int>(num_ISN,bin_count,0); 
	
	// int total_pkts=0;
	int *total_pkts = create_1D_array<int>(num_ISN,0);
	int temp_count=0;
	for(k=0;k<num_ISN;k++){
		for(i=0;i<bin_count;i++){
			for(j=0;j<m;j++){
				all_latency[k][i]+=latency_hist[k][j][i];
			}
			total_pkts[k]+=all_latency[k][i];
		}
		temp_count=0;
		for(i=0;i<bin_count;i++){
			temp_count+=all_latency[k][i];
			if(temp_count>total_pkts[k]*0.95 && nfp[k]==0){
				nfp[k]=i*bin_width;
			}
			if(temp_count>total_pkts[k]*0.99 && nnp[k]==0){
				nnp[k]=i*bin_width;
			}
		}
	}
	double **per_core_latency = create_2D_array<double>(num_ISN,m,0); 
	
	// double overall_latency=0;
	double *overall_latency = create_1D_array<double>(num_ISN,0); 
	for(k=0;k<num_ISN;k++){
		for(j=0;j<m;j++){
			for(i=0;i<bin_count;i++){
				per_core_latency[k][j]=per_core_latency[k][j]+i*bin_width*latency_hist[k][j][i];
			}
			overall_latency[k]+=per_core_latency[k][j];
			per_core_latency[k][j]=per_core_latency[k][j]/server_pkts_counter[k][j];
			
		}
	}
	
	// aggregator latency
	int total_pkts_agg=0;
	for(i=0;i<bin_count;i++){
		total_pkts_agg+=Agg_latency_hist[i];
	}
	int agg_nfp=0,agg_nnp=0;
	
	temp_count=0;
	for(i=0;i<bin_count;i++){
		
		temp_count+=Agg_latency_hist[i];
		if(temp_count>total_pkts_agg*0.95 && agg_nfp==0){
			agg_nfp=i*bin_width;
		}
		if(temp_count>total_pkts_agg*0.99 && agg_nnp==0){
			agg_nnp=i*bin_width;
		}
	}
	
	// Aggregator quality
	int total_pkts_quality_agg=0;
	double avg_quality=0;
	for(i=0;i<101;i++){
		total_pkts_quality_agg+=Agg_quality_hist[i];
	}
	for(i=0;i<101;i++){
		avg_quality+=Agg_quality_hist[i]*1.0*i;
	}
	avg_quality=avg_quality/total_pkts_quality_agg;
	
	// ISN drop
	int total_pkts_drop=0;
	double avg_drop=0;
	for(i=0;i<num_ISN+1;i++){
		total_pkts_drop+=Agg_drop_hist[i];
	}
	for(i=0;i<num_ISN+1;i++){
		avg_drop+=Agg_drop_hist[i]*1.0*i;
	}
	avg_drop=avg_drop/total_pkts_drop;
	
	/*print results*/
	if(!quiet){
		for(i=0;i<num_ISN;i++){
			printf("ISN %d\taverage latency %f\n",i,overall_latency[i]/pkt_processed[i]);
			printf("ISN %d\taverage busy ratio %f\n",i,overall_busy_ratio[i]);
			printf("ISN %d\taverage idle ratio %f\n",i,overall_idle_ratio[i]);
			printf("ISN %d\taverage wakeup ratio %f\n",i,overall_wakeup_ratio[i]);
			printf("ISN %d\taverage power %f\n",i,(Pa*(overall_busy_ratio[i]+overall_wakeup_ratio[i])+Pc*overall_idle_ratio[i])*m);
		}
	}
	// if(0){
		// printf("\n************per server***********\n\n");
		// printf("busy period count\t");
		// for(j=0;j<m;j++){
			// printf("%d\t",server_busy_counter[j]);
		// }
		// printf("\n");
		// printf("wake up count\t");
		// for(j=0;j<m;j++){
			// printf("%d\t",server_wakeup_counter[j]);
		// }
		// printf("\n");
		// for(j=0;j<m;j++){
			// printf("server %d: pkts : %d\t busy ratio: %f\t idle ratio: %f\t wakeup ratio: %f\tavg latency : %f\n",j,server_pkts_counter[j],per_core_busy_ratio[j],per_core_idle_ratio[j],per_core_wakeup_ratio[j],per_core_latency[j]);
			
		// }
	// }
	double total_power=0;
	for(i=0;i<num_ISN;i++){
		total_power+=(Pa*(overall_busy_ratio[i]+overall_wakeup_ratio[i])+Pc*overall_idle_ratio[i])*m;
	}
	if(!quiet){
		printf("%-5s\t%-6s\t%-6s\t%-12s\t%-12s\t%-12s\t%-7s\n","ISN","Util.","power","avg_latency","95th_latency","99th_latency","C_state");
		for(i=0;i<num_ISN;i++){
			printf("%-5d\t%1.4f\t%2.3f\t%7.4f\t%-12d\t%-12d\tC%-6d\n",i,overall_busy_ratio[i],
			(Pa*(overall_busy_ratio[i]+overall_wakeup_ratio[i])+Pc*overall_idle_ratio[i])*m,
			overall_latency[i]/pkt_processed[i],nfp[i],nnp[i],C_state);
		}
	}
	if(!quiet){
		printf("AGG:\t%-5d\t%d\t",num_ISN,m);
	}
	printf("%d\t%.4f\t%.4f\t%.4f\t%.4f\t%d\t%d\n",p,time_out_counter*1.0/Agg_pkt_processed,avg_drop,avg_quality,total_power,agg_nfp,agg_nnp);
	// printf("%d\t%.2f\t%f\t%f\t%d\t%d\t%d\t%d\t%d\tC%d\n",m,p,(Pa*(overall_busy_ratio+overall_wakeup_ratio)+Pc*overall_idle_ratio)*m,overall_latency/pkt_processed,nfp,nnp,agg_nfp,agg_nnp,0,C_state);
	
	return 0;

}




