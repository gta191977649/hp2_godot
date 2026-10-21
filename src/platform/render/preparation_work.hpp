#pragma once
// Shared CPU callback used by Godot native groups and native-only validation.
#include "mesh_prepare.hpp"
#include "submit_math.hpp"
#include <atomic>
#include <chrono>
#include <exception>
#include <algorithm>
namespace port::render {
struct MeshPreparationJob {MeshPrepareInput input;MeshPrepareOutput output;std::exception_ptr error;double duration=0;};
struct PreparationWork {
    std::vector<MeshPreparationJob> jobs;
    std::vector<SubmitMathInput> math_inputs;
    std::vector<SubmitMathOutput> math_outputs;
    std::vector<std::exception_ptr> math_errors;
    std::vector<double> math_durations;
    std::atomic<unsigned> active{0},peak{0};
    void configure_math(std::vector<SubmitMathInput> inputs,unsigned workers){
        math_inputs=std::move(inputs);math_outputs.resize(math_inputs.size());
        const auto chunks=std::min<std::size_t>(math_inputs.size(),workers?workers*2u:1u);
        math_errors.resize(chunks);math_durations.resize(chunks);
    }
    std::size_t tasks()const{return jobs.size()+math_errors.size();}
    static void run(void* data,std::uint32_t index) noexcept {
        auto& w=*static_cast<PreparationWork*>(data);const auto count=w.active.fetch_add(1)+1;
        auto peak=w.peak.load();while(peak<count&&!w.peak.compare_exchange_weak(peak,count)){}
        const auto begin=std::chrono::steady_clock::now();
        if(index<w.jobs.size()){
            auto& j=w.jobs[index];try{j.output=prepare_mesh(j.input);}catch(...){j.error=std::current_exception();}
            j.duration=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        }else{
            const auto chunk=index-w.jobs.size(),chunks=w.math_errors.size();
            try{for(auto i=w.math_inputs.size()*chunk/chunks;i<w.math_inputs.size()*(chunk+1)/chunks;++i)w.math_outputs[i]=prepare_submit_math(w.math_inputs[i]);}
            catch(...){w.math_errors[chunk]=std::current_exception();}
            w.math_durations[chunk]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        }
        w.active.fetch_sub(1);
    }
    void rethrow_errors()const{for(const auto& j:jobs)if(j.error)std::rethrow_exception(j.error);for(const auto& e:math_errors)if(e)std::rethrow_exception(e);}
};
} // namespace port::render
