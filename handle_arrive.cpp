#include "handle_arrive.h"
#include "template_impl.tpp"

int handle_arrive(double cur_time, double* event){
	
	static double time;
	static Pkt incoming_pkt, fetched_pkt;
	int i;
	
	time = cur_time; 

	error("pkt %d arrived at time %f\n",pkt_index,time);
	
	// determine the pkt service time
	double pkt_service_time=service_length[generate_iat(service_count,service_cdf)]*1000000;
	
	//fill up the pkt
	incoming_pkt.index=pkt_index;
	incoming_pkt.time_arrived=time;
	incoming_pkt.service_time=pkt_service_time*freq[0]/freq[select_f]*alpha+pkt_service_time*(1-alpha);
	incoming_pkt.time_finished=-1;
	
	// insert pkt into queue			
	queue->enQ(incoming_pkt);
	
	// update pkt index
	pkt_index++;
	

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
				
		error("pkt %d assigned to server %d\n",server[pick].cur_pkt.index,pick);
		error("pkt %d should depart at time %f\n",server[pick].cur_pkt.index,server[pick].time_finished);
		
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
	
	// determine the next pkt arrival time
	double inter_arrival;
	if(exponential==0)
		inter_arrival=arrival_length[generate_iat(arrival_count,arrival_cdf)]*1000000;
	else
		inter_arrival=expntl(Ta)*1000000;
		
	// update event 
	event[0] = time + inter_arrival; 
	event[1] = SIM_TIME;
	for(i=0;i<m;i++){
		if(server[i].time_finished<event[1])
			event[1]=server[i].time_finished;
	}
	
	return 0;
	
}