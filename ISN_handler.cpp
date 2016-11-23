#include "ISN_handler.h"
#include "template_impl.tpp"

int handle_arrive(double cur_time, double* event){
	
	static double time;
	static Pkt incoming_pkt, fetched_pkt;
	int i;
	
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
		if(server[i].state==0){
			map[index]=i;
			index++;
		}
	}
	int which_bin;
	
	if(index>0){ // free server found, server goes busy
		pick=map[rand_int(index)]; // randomly pick one idle server to process the pkt
		
		// log the idle period
		which_bin=floor((time-server[pick].time_arrived)/bin_width);
		if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
		if (which_bin > bin_count-1) which_bin=bin_count-1;
		server_idle_time_hist[pick][which_bin]++;
		server_idle_counter[pick]++;
		
		// activate server
		server[pick].state=1;
		server_wakeup_counter[pick]++;
		// fetch pkt from queue
		fetched_pkt=queue->deQ();
		// put pkt into the server
		server[pick].cur_pkt=fetched_pkt;
		server[pick].cur_pkt.handled=pick;
		server[pick].time_arrived=time+wake_up_latency;
		server[pick].time_finished=server[pick].time_arrived+server[pick].cur_pkt.service_time;
		error("%f\tISN fetched pkt %d\n",time,server[pick].cur_pkt.index);		
		error("%f\tpkt %d assigned to core %d\n",time,server[pick].cur_pkt.index,pick);
		error("%f\tpkt %d should depart at time %f\n",time,server[pick].cur_pkt.index,server[pick].time_finished);
		
		// check package state
		if(package.time_arrived!=0){
			which_bin=floor((time-package.time_arrived)/bin_width);
			if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
			if (which_bin > bin_count-1) which_bin=bin_count-1;
			package_idle_time_hist[which_bin]++;
			package.time_arrived=-1;
			package_idle_counter++;
		}

	} else{ // all server busy, do nothing
		
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
		if(server[i].time_finished<event[1])
			event[1]=server[i].time_finished;
	}
	
	return 0;
	
}

int handle_depart(double cur_time, double* event){
	
	int i,pick;
	static double time;
	static Pkt fetched_pkt;
	time = cur_time;
	// find which server has finished pkts
	pick=-1;
	for(i=0;i<m;i++){
		if(server[i].time_finished==event[1]){
			pick=i;
			break;
		}
	}
	if(pick<0) {// should not happen
		printf("error\n"); 
		return -1;
	} 
	error("%f\tpkt %d departed from server %d at time %f\n",time,server[pick].cur_pkt.index,pick,time);
	server[pick].cur_pkt.time_finished=time;
	// record latency
	int which_bin;
	which_bin=floor((server[pick].cur_pkt.time_finished-server[pick].cur_pkt.time_arrived)/bin_width);
	if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
	if (which_bin > bin_count-1) which_bin=bin_count-1;
	latency_hist[pick][which_bin]++;

	server_pkts_counter[pick]=server_pkts_counter[pick]+1;


	if(queue->getQlength()>0){ // there are pkts in the queue, imediatly assign to the server
		server[pick].state=1;
		fetched_pkt=queue->deQ();
		server[pick].cur_pkt=fetched_pkt;
		server[pick].cur_pkt.handled=pick;
		server[pick].time_finished=time+server[pick].cur_pkt.service_time;
		
		error("%f\tpkt %d assigned to server %d\n",time,server[pick].cur_pkt.index,pick);
		error("%f\tpkt %d should depart at time %f\n",time,server[pick].cur_pkt.index,server[pick].time_finished);
	} else{ // no queue pkts, server goes idle
		
		which_bin=floor((time-server[pick].time_arrived)/bin_width);
		if(which_bin<0) {printf("latency error %d\n",__LINE__); return 0;}
		if (which_bin > bin_count-1) which_bin=bin_count-1;
		server_busy_time_hist[pick][which_bin]++;
		
		server_busy_counter[pick]++;
		server[pick].state=0;
		server[pick].cur_pkt=NULL_PKT;
		server[pick].time_arrived=time;
		server[pick].time_finished=SIM_TIME;
		
		// check package state
		if(package_sleep!=0){
			check_package=0;
			for(i=0;i<m;i++){
				check_package=check_package+server[i].state;
			}
			if(check_package==0){ // package enter C6
				package.time_arrived=time;
			}
		}
		
	}

	event[1] = SIM_TIME;
	for(i=0;i<m;i++){
		if(server[i].time_finished<event[1])
			event[1]=server[i].time_finished;
	}
	
	return 0;

}