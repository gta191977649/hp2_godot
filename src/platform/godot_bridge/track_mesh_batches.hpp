#pragma once
#include "track_command_batches.hpp"
#include "platform/render/preparation_work.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <functional>
#include <memory>
namespace port::godot_bridge {
// Track-owned host cache. Clear before replacing the source mesh lookup's assets.
class TrackMeshBatchCache {
public:
    using MeshLookup=std::function<godot::Ref<godot::ArrayMesh>(std::size_t,std::size_t)>;
    using ArrayLookup=std::function<godot::Array(std::size_t,std::size_t)>;
    using CpuLookup=std::function<std::shared_ptr<const port::render::VehicleSurface>(std::size_t,std::size_t)>;
    using SourceReady=std::function<bool(std::size_t,std::size_t)>;
    explicit TrackMeshBatchCache(MeshLookup,std::size_t maximum_cached_bytes=64u*1024u*1024u,ArrayLookup={});
    ~TrackMeshBatchCache();
    godot::Ref<godot::ArrayMesh> acquire(const port::track::Frame&,const TrackCommandBatch&);
    void set_cpu_source(CpuLookup,SourceReady);
    // Main-thread snapshot, same-view join, then ordered acquire/upload. No jobs survive this call.
    void prepare_view(const port::track::Frame&,const std::vector<TrackCommandBatch>&,
        const std::vector<bool>& requested,unsigned workers,std::uint64_t source_frame,unsigned view,std::uint64_t scene_generation=0,std::vector<port::render::SubmitMathInput> math={});
    const port::render::SubmitMathOutput& submit_math(std::size_t index)const;
    godot::Array source_arrays(std::size_t solid,std::size_t surface);
    void finish_view();
    void note_source_upload(double wall_ms);
    godot::Dictionary preparation_stats()const;
    void clear();
    void trim_unreferenced();
    std::size_t bytes()const;
    std::size_t entries()const;
    std::size_t builds()const;
    // Logical CPU vertex/index payload submitted on cache misses, not driver traffic.
    std::size_t uploaded_payload_bytes()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
