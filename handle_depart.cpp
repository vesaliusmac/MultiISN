#include "handle_arrive.h"
#include "template_impl.tpp"

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
	error("pkt %d departed from server %d at time %f\n",server[pick].cur_pkt.index,pick,time);
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
		
		error("pkt %d assigned to server %d\n",server[pick].cur_pkt.index,pick);
		error("pkt %d should depart at time %f\n",server[pick].cur_pkt.index,server[pick].time_finished);
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
