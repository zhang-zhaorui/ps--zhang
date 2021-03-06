#include <iostream>
#include "ps/ps.h"
using namespace std;
using namespace ps;
#define ASYNC 1
#define SYNC 2
#define RATE 0.7
#define MAX_DIFF 1000
int method = 4;
template <class Val>
class KVServerDefaultHandle1 {      //functor，用与处理server收到的来自worker的请求
public:
    // req_meta是存储该请求的一些元信息，即请求来自于哪个节点，发送给哪个节点等等
    // req_data即发送过来的数据
    // server即指向当前server对象的指针
    void operator() (const KVMeta& req_meta, const KVPairs<Val>& req_data, KVServer<Val>* server) {
        size_t n = req_data.keys.size();
        int work_id = (req_meta.sender - 9)/2;
        
        std::cout << "worker id is " << work_id << "where it is push" << req_meta.push << std::endl;
        if (req_meta.push) { // push
            
        } else {            // pull
            
        }

        size_t cur_idx = 0;
        for (size_t i = 0;i < n; ++i) {

            Key key = req_data.keys[i];
            if(req_meta.push){ //push
                
                KVPairs<Val> res;
                CHECK_EQ(n, req_data.lens.size());
                int len = req_data.lens[i];
                
                if(grad[work_id].size() == 0){//第一次push，开辟空间
                    for(int work_init_itr = 0; work_init_itr < NumWorkers(); ++work_init_itr){
                        grad[work_init_itr] = vector<float>(len, 0);
                    }
                }


                ticks[work_id]++;
                
                for(int work_itr = 0; work_itr < NumWorkers(); ++work_itr) {
                    cur_idx = 0;
                    for(int idx = 0; idx < len; ++idx){
                        grad[work_itr][idx] += req_data.vals[cur_idx++];
                    }
                    grad_count[work_itr]++;
                }
                
                server->Response(req_meta, res);
            }
            else{ // pull
                if (method == 1) {
                    KVPairs<Val> res;
                    res.keys = req_data.keys;
                    res.lens.resize(res.keys.size());
    
                    res.lens[i] = grad[work_id].size();
                    for(int idx = 0; idx < res.lens[i]; ++idx){
                        res.vals.push_back(grad[work_id][idx]/grad_count[work_id]);
                        grad[work_id][idx] = 0;
                        
                    }
                    grad_count[work_id] = 0;
                    server->Response(req_meta, res);
                } else if (method == 2){
                    if(ticks[work_id] > meta_queue.size()) {
                        meta_queue.push_back(std::vector<KVMeta>());
                        data_queue.push_back(std::vector<KVPairs<Val> >());
                    }
    
                    meta_queue[ticks[work_id] - last_tick].push_back(req_meta);
    
                    data_queue[ticks[work_id] - last_tick].push_back(req_data);
    
                    if(meta_queue[ticks[work_id] - last_tick].size() == NumWorkers()) {
                        for(int meta_queue_tick_itr = 0; meta_queue_tick_itr < meta_queue[ticks[work_id] - last_tick].size(); ++meta_queue_tick_itr) {
    
                            KVPairs<Val> res;
                            res.keys = data_queue[ticks[work_id] - last_tick][meta_queue_tick_itr].keys;
                            res.lens.resize(data_queue[ticks[work_id] - last_tick][meta_queue_tick_itr].keys.size());
    
                            res.lens[i] = grad[work_id].size();
                            //std::cout << "the i is hahaha" << i << std::endl;
                            int recv_id = (meta_queue[ticks[work_id] - last_tick][meta_queue_tick_itr].sender - 9)/2;
                            
                            for(int idx = 0; idx < res.lens[i]; ++idx){
    
                                res.vals.push_back(grad[recv_id][idx]/grad_count[recv_id]);
                                grad[recv_id][idx] = 0;
                                
                            }
                            grad_count[recv_id] = 0;
                            server->Response(meta_queue[ticks[work_id] - last_tick][meta_queue_tick_itr], res);
                        }
                    }
                }else if (method == 3) {

                    for(int vec_itr = 0; vec_itr < meta_vec.size(); ++vec_itr) {

                        bool should_be_send = true;
                        int work_itr_tick = ticks[(meta_vec[vec_itr].sender - 9)/2];
                       
                        for(int tick_itr = 0; tick_itr < NumWorkers(); ++tick_itr) {
                            if(work_itr_tick - ticks[tick_itr] > MAX_DIFF) {
                                should_be_send = false;
                                break;
                            }
                        }

                        if(should_be_send) {
                            KVPairs<Val> res;
                            res.keys = data_vec[vec_itr].keys;
                            res.lens.resize(data_vec[vec_itr].keys.size());
            
                            res.lens[i] = grad[work_id].size();

                            int recv_id = (meta_vec[vec_itr].sender - 9)/2;
                            for(int idx = 0; idx < res.lens[i]; ++idx){
                                res.vals.push_back(grad[recv_id][idx]/grad_count[recv_id]);
                                grad[recv_id][idx] = 0;
                                
                                                            
                            }
                            grad_count[recv_id] = 0;
                            std::cout << "Now we send to worker" << recv_id << std::endl;
                            std::cout << "s the ticks is ";
                            for(auto v:ticks) {
                                cout << v << "  ";
                            }
                            std::cout << std::endl;
                            server->Response(meta_vec[vec_itr], res);

                            std::cout << "before erase";
                            for(auto v:meta_vec) {
                                cout << v.sender << " ";
                            }
                            std::cout << std::endl;
                            meta_vec.erase(meta_vec.begin() + vec_itr);
                            data_vec.erase(data_vec.begin() + vec_itr);
                            std::cout << "after erase";
                            for(auto v:meta_vec) {
                                cout << v.sender << " ";
                            }
                            std::cout << std::endl;
                            
                        }
                    }
                    int my_tick = ticks[work_id];

                    for(int work_itr = 0; work_itr < NumWorkers(); work_itr++) {
                        if((my_tick - ticks[work_itr]) > MAX_DIFF) {
                            meta_vec.push_back(req_meta);
                            data_vec.push_back(req_data);
                            std::cout << "we need to wait as the ticks is ";
                            for(auto v:ticks) {
                                cout << v << "  ";
                            }
                            std::cout << std::endl;
                            return;
                        }
                    }
                    
                    KVPairs<Val> res;
                    res.keys = req_data.keys;
                    res.lens.resize(res.keys.size());
    
                    res.lens[i] = grad[work_id].size();
                    for(int idx = 0; idx < res.lens[i]; ++idx){
                        res.vals.push_back(grad[work_id][idx]/grad_count[work_id]);
                        grad[work_id][idx] = 0;
                        
                    }
                    grad_count[work_id] = 0;
                    server->Response(req_meta, res);
                } else if (method == 4) {
                    
                    if(meta_vec.size() < worker_needed) {
                        meta_vec.push_back(req_meta);
                        data_vec.push_back(req_data);
                        std::cout << "push push" << meta_vec.size() << std::endl;
                        return;
                    } else {
                        meta_vec.push_back(req_meta);
                        data_vec.push_back(req_data);
                        for(int vec_itr = 0; vec_itr < worker_needed; ++vec_itr) {
                            
                            KVPairs<Val> res;
                            res.keys = data_vec[vec_itr].keys;
                            res.lens.resize(data_vec[vec_itr].keys.size());
                            std::cout << "be heree1" << std::endl;
                            int recv_id = (meta_vec[vec_itr].sender - 9)/2;
                            res.lens[i] = grad[recv_id].size();
                            std::cout << "be heree3" << std::endl;
                            
                            
                            for(int idx = 0; idx < res.lens[i]; ++idx){
                                res.vals.push_back(grad[recv_id][idx]/grad_count[recv_id]);
                                grad[recv_id][idx] = 0;
                            }
                            grad_count[recv_id] = 0;
                            std::cout << "be heree5" << std::endl;
                            
                            server->Response(meta_vec[vec_itr], res);
                            std::cout << "be heree6" << std::endl;
                            
                        }
                        std::cout << "be heree7" << std::endl;
                        
                        meta_vec.erase(meta_vec.begin(), meta_vec.begin() + worker_needed);
                        std::cout << "be heree8" << std::endl;
                        
                        data_vec.erase(data_vec.begin(), data_vec.begin() + worker_needed);
                        std::cout << "be heree9" << std::endl;
                        
                    }
                }
            }
        }
        
    }
private:
    std::vector<std::vector<float> > grad = std::vector<std::vector<float> >(NumWorkers());
    std::vector<int> grad_count = std::vector<int >(NumWorkers());
    std::vector<int> ticks =  std::vector<int>(NumWorkers(), 0);
    std::vector<std::vector<KVMeta> > meta_queue;
    std::vector<std::vector<KVPairs<Val> > > data_queue;

    std::vector<KVMeta> meta_vec;
    std::vector<KVPairs<Val> > data_vec;
    int last_tick = 1;
    const int worker_needed = NumWorkers() * RATE;
    
};

void StartServer() {
    if (!IsServer()) return;
    cout << "num of workers[" << NumWorkers() << "]" << endl;
    cout << "num of servers[" << NumServers() << "]" << endl;
    auto server = new KVServer<float>(0);
    server->set_request_handle(KVServerDefaultHandle1<float>());   //注册functor
    RegisterExitCallback([server](){ delete server; });
}

int main(int argc, char* argv[]) {
    StartServer();
    Start();    //启动,Postoffice::start()
    Finalize(); //结束。每个节点都需要执行这个函数。
    return 3;
}
