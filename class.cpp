#include "common.h"
#include "class.h"



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
