#include "common.h"
#include "class.h"
#include <algorithm>


Pkt::Pkt(){
	index=-1;
    time_arrived=-1;
	service_time=-1;
	time_finished=-1;
	handled=-1;
	// response_scores = new double[top_k];
	for(int i=0;i<100;i++){
		response_scores[i]=-1;
	}
}

Server::Server(){
	cur_pkt=NULL_PKT;
	time_arrived=0;
	time_finished=SIM_TIME;
	state=0;
}

Package::Package(){
	time_arrived=0;
	time_finished=SIM_TIME;
	state=0;
}

Queue::Queue(){
	head_ptr=0;
	tail_ptr=0;
	queue_elements_count=0;
}

int Queue::enQ(Pkt in){
	if(queue_elements_count>=Queue_length){
		return -1;
	}else{
	elements[tail_ptr]=in;
	tail_ptr=(tail_ptr+1)%Queue_length;
	queue_elements_count++;
		return 0;
	}
}

Pkt Queue::deQ(){
	if(queue_elements_count<1){
		return NULL_PKT;
	}
	Pkt temp=elements[head_ptr];
	head_ptr=(head_ptr+1)%Queue_length;
	queue_elements_count--;
	return temp;
}

int Queue::getQlength(){
	return queue_elements_count;
}

Agg_wait_list::Agg_wait_list(){
	for(int i=0;i<Queue_length;i++){
		index[i]=-1;
		time_arrived[i]=-1;
		time_expired[i]=-1;
		counter[i]=0;
		score_counter[i]=0;
		for(int j=0;j<16*100;j++){
			received_scores[i][j]=-1;
		}
	}
	wait_length=0;
}

int Agg_wait_list::add(int which, double time){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==-1){
			index[i]=which;
			time_arrived[i]=time;
			time_expired[i]=time+agg_timeout;
			wait_length++;
			return 0;
		}
	}
	return -1;
}

int Agg_wait_list::insert(Pkt pkt){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==pkt.index){
			counter[i]++;
			for(int j=0;j<100;j++){
				received_scores[i][score_counter[i]]=pkt.response_scores[j];
				score_counter[i]++;
			}
			return 0;
		}
	}
	return -1;
}


int Agg_wait_list::remove(int which){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			index[i]=-1;
			time_arrived[i]=-1;
			time_expired[i]=-1;
			counter[i]=0;
			score_counter[i]=0;
			for(int j=0;j<16*100;j++){
				received_scores[i][j]=-1;
			}
			wait_length--;
			return 0;
		}
	}
	return -1;
}
bool myfunction (double i,double j) { return (i>j); }


int Agg_wait_list::sort_score(int which){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			// printf("before\t");
			// for(int j=0;j<16*100;j++){
				// printf("%f\t",received_scores[i][j]);
			// }
			// printf("\n");
			// std::sort(received_scores[i],received_scores[i]+16*100);
			std::sort(received_scores[i],received_scores[i]+16*100,myfunction);
			// printf("after\t");
			// for(int j=0;j<16*100;j++){
				// printf("%f\t",received_scores[i][j]);
			// }
			// printf("\n");
			return 0;
		}
	}
	return -1;
}

double Agg_wait_list::getScore(int which, int howmany){
	double temp=0;
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			for(int j=0;j<howmany;j++){
				if(received_scores[i][j]>0)
					temp+=received_scores[i][j];
			}
			return temp;
		}
	}
	return -1;
}

int Agg_wait_list::find_next_timeout(){
	double min=DBL_MAX;
	int min_index=-1;
	for(int i=0;i<Queue_length;i++){
		if(min>time_expired[i] && time_expired[i]>0){
			min=time_expired[i];
			min_index=index[i];
		}
	}
	return min_index;
}

double Agg_wait_list::getTime(int which){
	if(which==-1) return SIM_TIME;
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			return time_expired[i];
		}
	}	
	return -1;
}

int Agg_wait_list::getCounter(int which){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			return counter[i];
		}
	}	
	return -1;
}

double Agg_wait_list::getArriveTime(int which){
	for(int i=0;i<Queue_length;i++){
		if(index[i]==which){
			return time_arrived[i];
		}
	}
	return -1;
}
