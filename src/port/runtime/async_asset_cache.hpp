#pragma once
// TASK-0105: port-only single-worker, bounded immutable-template cache.
// Promise futures never wait for worker completion when a ticket is discarded.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <utility>

namespace port::runtime {
template<class Key,class Value> class AsyncAssetCache {
public:
    struct Result {std::shared_ptr<Value> value;bool cache_hit=false;double load_ms=0;};
    struct Ticket {
        std::shared_ptr<std::atomic<bool>> cancelled;
        std::future<Result> result;
        void cancel(){if(cancelled)cancelled->store(true);}
    };
    using Load=std::function<std::shared_ptr<Value>(const Key&)>;
    using Weight=std::function<std::size_t(const Value&)>;
    AsyncAssetCache(Load load,Weight weight,std::size_t entries,std::size_t bytes)
        :load_(std::move(load)),weight_(std::move(weight)),entry_limit_(entries),byte_limit_(bytes),worker_([this]{run();}){}
    ~AsyncAssetCache(){
        {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;for(auto& job:jobs_)job.cancelled->store(true);}
        wake_.notify_one();worker_.join();
    }
    Ticket request(Key key){
        Job job{std::move(key),std::make_shared<std::atomic<bool>>(false),{}};
        Ticket out{job.cancelled,job.promise.get_future()};
        {std::lock_guard<std::mutex> lock(mutex_);jobs_.push_back(std::move(job));}
        wake_.notify_one();return out;
    }
private:
    struct Job {Key key;std::shared_ptr<std::atomic<bool>> cancelled;std::promise<Result> promise;};
    struct Entry {std::shared_ptr<const Value> value;std::size_t bytes;std::uint64_t age;};
    Load load_;Weight weight_;std::size_t entry_limit_,byte_limit_,bytes_=0;
    std::map<Key,Entry> cache_;std::uint64_t age_=0;
    std::mutex mutex_;std::condition_variable wake_;std::deque<Job> jobs_;bool stopping_=false;
    std::thread worker_;
    void run(){for(;;){
        Job job;
        {std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[&]{return stopping_||!jobs_.empty();});
            if(stopping_)return;
            job=std::move(jobs_.front());jobs_.pop_front();}
        const auto start=std::chrono::steady_clock::now();Result result;
        try{
            if(!job.cancelled->load()){
                auto found=cache_.find(job.key);result.cache_hit=found!=cache_.end();
                std::shared_ptr<const Value> value;
                if(found!=cache_.end()){found->second.age=++age_;value=found->second.value;}
                else{
                    value=load_(job.key);if(!value)throw std::runtime_error("Asset loader returned no value");
                    const auto size=weight_(*value);
                    if(entry_limit_ && size<=byte_limit_){
                        while(!cache_.empty() && (cache_.size()>=entry_limit_ || bytes_+size>byte_limit_)){
                            auto oldest=cache_.begin();for(auto i=cache_.begin();i!=cache_.end();++i)if(i->second.age<oldest->second.age)oldest=i;
                            bytes_-=oldest->second.bytes;cache_.erase(oldest);
                        }
                        cache_.emplace(job.key,Entry{value,size,++age_});bytes_+=size;
                    }
                }
                // Each consumer owns independent mutable pose state. Copy only on worker.
                if(!job.cancelled->load())result.value=std::make_shared<Value>(*value);
            }
            result.load_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            job.promise.set_value(std::move(result));
        }catch(...){job.promise.set_exception(std::current_exception());}
    }}
};
}
