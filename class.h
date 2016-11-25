

#ifndef CLASS_H
#define CLASS_H
class Pkt {
	public:
	Pkt();
    int index;
    double time_arrived;
	double service_time;
	double time_finished;
	int handled;
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
	int insert(int which);
	int remove(int which);
	
	int find_next_timeout();
	
	double getTime(int which);
	int getCounter(int which);
	double getArriveTime(int which);
	
	private:
	int index[Queue_length];
	double time_arrived[Queue_length];
	double time_expired[Queue_length];
	int counter[Queue_length];
	int wait_length;

};
extern Pkt NULL_PKT;
#endif
