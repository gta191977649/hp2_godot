// TASK-0099: port memory clone/destroy baseline, NOT a race/FPS or GPU benchmark.
#include "game_core/port/driving/original_image.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>
namespace port::diagnostics {
int memory_copy_baseline(const std::filesystem::path& root){
    const auto memory=driving::read_debug_image(driving::debug_image_path(root));
    const auto footprint=memory.footprint();
    constexpr int warmup=120,count=600;
    volatile std::uint32_t observed=0;
    std::vector<double> samples;samples.reserve(count);
    for(int frame=-warmup;frame<count;++frame){
        const auto begin=std::chrono::steady_clock::now();
        {
            driving::Memory copy=memory;
            observed^=copy.word(0x311f78);
        }
        const auto end=std::chrono::steady_clock::now();
        if(frame>=0)samples.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
    }
    auto sorted=samples;std::sort(sorted.begin(),sorted.end());
    const auto p99=sorted[static_cast<std::size_t>(std::ceil(.99*count))-1];
    const auto mean=std::accumulate(samples.begin(),samples.end(),0.0)/count;
    std::cout<<std::setprecision(9)
        <<"{\"scope\":\"port memory image clone and destroy only; no simulation/rendering\","
        <<"\"behavior_sha256\":\""<<driving::debug_elf_sha256<<"\","
        <<"\"warmup\":"<<warmup<<",\"samples\":"<<count
        <<",\"regions\":"<<footprint.regions<<",\"payload_bytes_per_copy\":"<<footprint.payload_bytes
        <<",\"mapped_owned_allocations\":"<<footprint.owned_allocations
        <<",\"heap_allocation_count_measured\":false,\"gpu_time_available\":false"
        <<",\"mean_ms\":"<<mean<<",\"p50_ms\":"<<sorted[count/2]
        <<",\"p99_ms\":"<<p99<<",\"max_ms\":"<<sorted.back()
        <<",\"samples_ms\":[";
    for(std::size_t i=0;i<samples.size();++i){if(i)std::cout<<',';std::cout<<samples[i];}
    std::cout<<"]}\n";
    (void)observed; // Observable reads; no game-state payload is exported.
    return 0;
}
}
int main(int argc,char** argv){
    return port::diagnostics::memory_copy_baseline(argc>1?std::filesystem::path(argv[1]):std::filesystem::current_path());
}
