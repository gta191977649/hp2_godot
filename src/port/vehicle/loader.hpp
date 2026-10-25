#pragma once
#include "model.hpp"
#include "game_core/port/not_implemented.hpp"
#include <atomic>
#include <deque>
#include <condition_variable>
#include <future>
#include <thread>
#include <chrono>

namespace port::vehicle {
// Port-side single-worker session. No Godot objects may enter this service.
class Loader {
public:
    struct Result {std::unique_ptr<Model> model;std::string error;double cpu_ms=0,read_ms=0,geometry_decode_ms=0,texture_decode_ms=0,binding_ms=0;std::uint64_t file_reads=0;std::uint64_t geometry_reads=0,geometry_hits=0,texture_decodes=0,texture_hits=0;std::map<int,std::array<float,3>> previews;std::map<int,std::string> preview_errors;};
    struct Ticket {std::shared_ptr<std::atomic<bool>> cancelled;std::future<Result> result;};
    Loader(){thread_=std::thread([this]{run();});}
    ~Loader(){{std::lock_guard<std::mutex> lock(mutex_);stopping_=true;}wake_.notify_one();thread_.join();}
    Ticket request(std::string root,std::string path,int car,int skin,bool lods,bool blur=false,std::uint32_t frontend_solid=0){
        Job job;job.root=std::move(root);job.path=std::move(path);job.car=car;job.skin=skin;job.lods=lods;job.blur=blur;job.cancelled=std::make_shared<std::atomic<bool>>(false);
        job.frontend_solid=frontend_solid;Ticket ticket{job.cancelled,job.promise.get_future()};
        {std::lock_guard<std::mutex> lock(mutex_);jobs_.push_back(std::move(job));}wake_.notify_one();return ticket;
    }
private:
    struct TimedSource:assets::Source {
        assets::DirectorySource directory;std::vector<std::uint8_t>& global;double milliseconds=0;std::uint64_t reads=0;
        TimedSource(const std::string& root,std::vector<std::uint8_t>& shared_global):directory(root),global(shared_global){}
        const std::string& root()const override{return directory.root();}
        bool probe(const std::string& path)override{return directory.probe(path);}
        std::vector<std::uint8_t> read(const std::string& path)override{if(path=="GLOBAL/GLOBALB.BUN"&&!global.empty())return global;const auto start=std::chrono::steady_clock::now();auto data=directory.read(path);milliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++reads;if(path=="GLOBAL/GLOBALB.BUN")global=data;return data;}
    };
    struct Job {std::string root,path;int car=-1,skin=1;bool lods=false,blur=false;std::uint32_t frontend_solid=0;std::shared_ptr<std::atomic<bool>> cancelled;std::promise<Result> promise;};
    std::mutex mutex_;std::condition_variable wake_;std::deque<Job> jobs_;bool stopping_=false;std::thread thread_;
    std::string preview_root_;std::map<int,std::map<int,std::array<float,3>>> previews_;
    std::shared_ptr<ModelCache> cache_=std::make_shared<ModelCache>();
    void run(){for(;;){
        Job job;{std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[&]{return stopping_||!jobs_.empty();});if(stopping_)return;job=std::move(jobs_.front());jobs_.pop_front();}
        Result result;const auto start=std::chrono::steady_clock::now();
        if(!*job.cancelled){
            // hp2::guard is the only boundary for untranslated recovered paths.
            try{const auto missing=hp2::guard([&]{
                if(cache_->root!=job.root){cache_=std::make_shared<ModelCache>();cache_->root=job.root;}
                TimedSource source(job.root,cache_->global);auto model=std::make_unique<Model>(cache_);model->include_lods(job.lods);model->include_wheel_blur(job.blur);
                const auto reads=cache_->reads,hits=cache_->hits,decodes=cache_->textures->decodes,texture_hits=cache_->textures->hits;
                const auto geometry_time=cache_->geometry_decode_ms,texture_time=cache_->textures->decode_ms,binding_time=cache_->binding_ms;
                if(job.frontend_solid)model->load_frontend_solid(source,job.frontend_solid);
                else if(job.car>=0)model->load_id(source,job.car,job.skin);else model->load(source,job.path);
                if(preview_root_!=job.root){previews_.clear();preview_root_=job.root;}
                if(!job.frontend_solid && cache_->catalog.empty()){Catalog catalog(source);catalog.load();cache_->catalog=catalog.cars();}
                if(!*job.cancelled){
                    if(!job.frontend_solid){
                    result.previews=previews_[model->car_id()];
                    for(const auto& car:cache_->catalog)if(int(car.id)==model->car_id())for(const auto& skin:car.skins){
                        if(*job.cancelled)break;
                        if(result.previews.count(int(skin.id)))continue;
                        try{const auto missing_preview=hp2::guard([&]{Model preview(cache_);preview.include_lods(false);preview.load_id(source,int(car.id),int(skin.id));result.previews[int(skin.id)]=preview.preview_color();});if(!missing_preview.empty())result.preview_errors[int(skin.id)]=missing_preview;}catch(const std::exception& error){result.preview_errors[int(skin.id)]=error.what();}
                    }
                    previews_[model->car_id()]=result.previews;
                    }
                    result.model=std::move(model);
                }
                result.geometry_reads=cache_->reads-reads;result.geometry_hits=cache_->hits-hits;result.texture_decodes=cache_->textures->decodes-decodes;result.texture_hits=cache_->textures->hits-texture_hits;result.geometry_decode_ms=cache_->geometry_decode_ms-geometry_time;result.texture_decode_ms=cache_->textures->decode_ms-texture_time;result.binding_ms=cache_->binding_ms-binding_time;result.read_ms=source.milliseconds;result.file_reads=source.reads;
            });if(!missing.empty())result.error=missing;
            }catch(const std::exception& error){result.error=error.what();result.model.reset();}
        }
        result.cpu_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();job.promise.set_value(std::move(result));
    }}
};
}
