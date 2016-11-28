#include "parameter.h"

#ifndef CLASS_H
#define CLASS_H
class Pkt {
	public:
	Pkt();
    int index;
    double time_arrived;
	double service_time;
	double time_finished;
	int disable_drop;
	int reQuery_response;
	// int handled;
	int which_ISN;
	int reply_drop;
	
	double response_scores[100];
};

class Server {

	public:
	Server();
    // int index;
	// int cur_pkt_index;
	Pkt cur_pkt;
    double time_arrived;
	double time_finished;
	int state; // 0 idle 1 busy
};


class Package
{
	public:
	Package();
    double time_arrived;
	double time_finished;
	int state; // 0 idle 1 busy
};

class Queue
{
	public:
	Queue();
    int enQ(Pkt in);
	
	Pkt deQ();
	
	int getQlength();
	
	private:
	Pkt elements[Queue_length];
	int head_ptr;
	int tail_ptr;
	int queue_elements_count;
	
	
};

class Agg_wait_list{
	public:
	Agg_wait_list();
	int add(int which, double expired);
	int insert(Pkt pkt);
	// int register_reply(int which);
	int resetCollect(int which);
	int remove(int which);
	int sort_score(int which);
	double getScore(int which, int howmany);
	
	int find_next_timeout();
	int find_next_collect();
	
	double getTime(int which);
	double getCollect(int which);
	int getCounter(int which);
	int getDropCounter(int which);
	int* getDropArray(int which);
	double getArriveTime(int which);
	
	private:
	int index[Queue_length];
	double time_arrived[Queue_length];
	double time_expired[Queue_length];
	double time_collect[Queue_length];
	int score_counter[Queue_length];
	double received_scores[Queue_length][num_ISN*100];
	int collected_ISN[Queue_length][num_ISN];
	int counter[Queue_length];
	int drop_counter[Queue_length];
	int wait_length;

};
extern Pkt NULL_PKT;
#endif
