#include "ISN_handler.h"
#include "template_impl.tpp"

int handle_arrive(int which_ISN, double cur_time, double* event){
	
	static double time;
	static Pkt incoming_pkt, fetched_pkt;
	int i;
	static double service_time_temp;
	time = cur_time; 

	
	
	//// determine the pkt service time
	//double pkt_service_time=service_length[generate_iat(service_count,service_cdf)]*1000000;
	//
	////fill up the pkt
	//incoming_pkt.index=pkt_index;
	//incoming_pkt.time_arrived=time;
	//incoming_pkt.service_time=pkt_service_time*freq[0]/freq[select_f]*alpha+pkt_service_time*(1-alpha);
	//incoming_pkt.time_finished=-1;
	//
	//// insert pkt into queue			
	//queue->enQ(incoming_pkt);
	//
	//// update pkt index
	//pkt_index++;
	

	// check if any avaliable server
	int index=0;
	for(i=0;i<m;i++){
		if(server[which_ISN][i].state==0){
			map[index]=i;
			index++;
		}
	}
	int which_bin;
	
	if(index>0){ // free server found, server goes busy
		pick=map[rand_int(index)]; // randomly pick one idle server to process the pkt
		
		// log the idle period
		which_bin=floor((time-server[which_ISN][pick].time_arrived)/bin_width);
		if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
		if (which_bin > bin_count-1) which_bin=bin_count-1;
		server_idle_time_hist[which_ISN][pick][which_bin]++;
		server_idle_counter[which_ISN][pick]++;
		
		// activate server
		server[which_ISN][pick].state=1;
		server_wakeup_counter[which_ISN][pick]++;
		// fetch pkt from queue
		fetched_pkt=ISN_queue[which_ISN].deQ();
		if(fetched_pkt.index==-1){
			printf("deQ error\n");
			return -1;
		}
		// determine pkt service time
		service_time_temp=query_service_time[fetched_pkt.index%num_line][which_ISN]*1000;
		if(service_time_temp>ISN_timeout){// too long, dropped
			fetched_pkt.service_time=ISN_prediction_overhead;
			fetched_pkt.reply_drop=1;
		}else{
			fetched_pkt.service_time=service_time_temp;
			fetched_pkt.reply_drop=0;
		}
		
		
		// put pkt into the server
		server[which_ISN][pick].cur_pkt=fetched_pkt;
		server[which_ISN][pick].time_arrived=time+wake_up_latency;
		server[which_ISN][pick].time_finished=server[which_ISN][pick].time_arrived+server[which_ISN][pick].cur_pkt.service_time;
		
		error("%f\tISN %d fetched pkt %d\n",time,which_ISN,server[which_ISN][pick].cur_pkt.index);		
		error("%f\tpkt %d assigned to core %d\n",time,server[which_ISN][pick].cur_pkt.index,pick);
		error("%f\tpkt %d should depart at time %f\n",time,server[which_ISN][pick].cur_pkt.index,server[which_ISN][pick].time_finished);
		
		// check package state
		if(package_sleep!=0){
			if(package[which_ISN].time_arrived!=0){
				which_bin=floor((time-package[which_ISN].time_arrived)/bin_width);
				if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
				if (which_bin > bin_count-1) which_bin=bin_count-1;
				package_idle_time_hist[which_ISN][which_bin]++;
				package[which_ISN].time_arrived=-1;
				package_idle_counter[which_ISN]++;
			}
		}

	} else{ // all server busy, do nothing
		error("%f\tall cores in ISN %d are busy\n",time,which_ISN);	
	}
	
	//// determine the next pkt arrival time
	//double inter_arrival;
	//if(exponential==0)
	//	inter_arrival=arrival_length[generate_iat(arrival_count,arrival_cdf)]*1000000;
	//else
	//	inter_arrival=expntl(Ta)*1000000;
	//	
	//// update event 
	//event[0] = time + inter_arrival; 
	event[0] = SIM_TIME;
	event[1] = SIM_TIME;
	for(i=0;i<m;i++){
		if(server[which_ISN][i].time_finished<event[1])
			event[1]=server[which_ISN][i].time_finished;
	}
	
	return 0;
	
}

int handle_depart(int which_ISN, double cur_time, double* event, double* Agg_event){
	
	int i,pick;
	static double time;
	static Pkt fetched_pkt, departed_pkt;
	time = cur_time;
	// find which server has finished pkts
	pick=-1;
	for(i=0;i<m;i++){
		if(server[which_ISN][i].time_finished==event[1]){
			pick=i;
			break;
		}
	}
	if(pick<0) {// should not happen
		printf("error\n"); 
		return -1;
	} 
	error("%f\tpkt %d departed from ISN %d server %d\n",time,server[which_ISN][pick].cur_pkt.index,which_ISN,pick);
	server[which_ISN][pick].cur_pkt.time_finished=time;
	
	// put the matched docs into response
	int result_counter=0;
	if(server[which_ISN][pick].cur_pkt.reply_drop==1){
	
	}else{
		for(i=0;i<row;i++){
			if(which_ISN==result_shard[server[which_ISN][pick].cur_pkt.index%num_line][i] && result_counter<top_k){
				server[which_ISN][pick].cur_pkt.response_scores[result_counter]=scores[server[which_ISN][pick].cur_pkt.index%num_line][i];
				result_counter++;
			}
		}
	}
	error("%f\tISN %d has %d match docs for pkt %d\n",time,which_ISN,result_counter,departed_pkt.index);
	
	departed_pkt=server[which_ISN][pick].cur_pkt;
	error("%f\tISN %d send response %d to Aggregator %f\t%f\n",time,which_ISN,departed_pkt.index,departed_pkt.time_finished,departed_pkt.time_arrived);
	// printf("%d\n",Agg_receive_queue->getQlength());
	if(Agg_receive_queue->enQ(departed_pkt)<0){
		printf("agg enQ error %d\n",departed_pkt.index);
		return -1;
	}
	
	// record latency
	int which_bin;
	which_bin=floor((server[which_ISN][pick].cur_pkt.time_finished-server[which_ISN][pick].cur_pkt.time_arrived)/bin_width);
	if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
	if (which_bin > bin_count-1) which_bin=bin_count-1;
	latency_hist[which_ISN][pick][which_bin]++;

	server_pkts_counter[which_ISN][pick]++;


	if(ISN_queue[which_ISN].getQlength()>0){ // there are pkt(s) in the queue, immediately assign to the server
		server[which_ISN][pick].state=1;
		fetched_pkt=ISN_queue[which_ISN].deQ();
		if(fetched_pkt.index==-1){
			printf("deQ error\n");
			return -1;
		}
		server[which_ISN][pick].cur_pkt=fetched_pkt;
		// server[which_ISN][pick].cur_pkt.handled=pick;
		server[which_ISN][pick].time_finished=time+server[which_ISN][pick].cur_pkt.service_time;
		
		error("%f\tpkt %d assigned to server %d\n",time,server[which_ISN][pick].cur_pkt.index,pick);
		error("%f\tpkt %d should depart at time %f\n",time,server[which_ISN][pick].cur_pkt.index,server[which_ISN][pick].time_finished);
	} else{ // no queue pkts, server goes idle
		
		which_bin=floor((time-server[which_ISN][pick].time_arrived)/bin_width);
		if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
		if (which_bin > bin_count-1) which_bin=bin_count-1;
		server_busy_time_hist[which_ISN][pick][which_bin]++;
		
		server_busy_counter[which_ISN][pick]++;
		server[which_ISN][pick].state=0;
		server[which_ISN][pick].cur_pkt=NULL_PKT;
		server[which_ISN][pick].time_arrived=time;
		server[which_ISN][pick].time_finished=SIM_TIME;
		
		// check package state
		if(package_sleep!=0){
			check_package=0;
			for(i=0;i<m;i++){
				check_package=check_package+server[which_ISN][i].state;
			}
			if(check_package==0){ // package enter C6
				package[which_ISN].time_arrived=time;
			}
		}
		
	}

	event[1] = SIM_TIME;
	for(i=0;i<m;i++){
		if(server[which_ISN][i].time_finished<event[1])
			event[1]=server[which_ISN][i].time_finished;
	}
	Agg_event[1] = time;
	
	return 0;

}