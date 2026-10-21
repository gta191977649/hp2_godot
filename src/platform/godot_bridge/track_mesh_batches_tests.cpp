#include "track_mesh_batches_tests.hpp"
#include "track_mesh_batches.hpp"
#include <godot_cpp/variant/array.hpp>
#include <cstring>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
godot::PackedByteArray normal_bytes(const godot::Variant& custom){
    if(custom.get_type()==godot::Variant::PACKED_BYTE_ARRAY)return custom;
    const godot::PackedFloat32Array floats=custom;godot::PackedByteArray bytes;bytes.resize(floats.size()*sizeof(float));std::memcpy(bytes.ptrw(),floats.ptr(),bytes.size());return bytes;
}
godot::Ref<godot::ArrayMesh> source_mesh(bool second){
    godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);
    const float x=second?10.f:0.f;
    arrays[godot::Mesh::ARRAY_VERTEX]=godot::PackedVector3Array{{x,0,0},{x+1,0,0},{x,1,0}};
    arrays[godot::Mesh::ARRAY_TEX_UV]=godot::PackedVector2Array{{0,0},{1,0},{0,1}};
    arrays[godot::Mesh::ARRAY_COLOR]=godot::PackedColorArray{{8.f/255,120.f/255,248.f/255,128.f/255},{1,0,0,1},{0,1,0,0}};
    // Nonunit values and signedzero ensure CUSTOM0 is not treated as a normalized normal.
    arrays[godot::Mesh::ARRAY_CUSTOM0]=godot::PackedFloat32Array{.023622f,-0.f,.992126f,0,-.75f,.125f,1.5f,0,2,3,4,0};
    arrays[godot::Mesh::ARRAY_INDEX]=godot::PackedInt32Array{0,1,2};
    godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},std::uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);return mesh;
}
}
godot::Dictionary test_track_mesh_batch_cache(){godot::Dictionary result;
 try{
    const std::array<godot::Ref<godot::ArrayMesh>,2> sources{{source_mesh(false),source_mesh(true)}};
    port::track::Frame frame;frame.commands.resize(2);frame.commands[1].solid=1;
    TrackCommandBatch batch;batch.vertices=6;batch.indices=6;batch.subranges={{0,0,3,0,3},{1,3,3,3,3}};
    TrackMeshBatchCache cache([&](std::size_t solid,std::size_t){return sources.at(solid);},0);
    auto merged=cache.acquire(frame,batch);require(merged.is_valid()&&merged->get_surface_count()==1,"Merged surface missing");
    const auto actual=merged->surface_get_arrays(0);
    const godot::PackedVector3Array p=actual[godot::Mesh::ARRAY_VERTEX];const godot::PackedVector2Array uv=actual[godot::Mesh::ARRAY_TEX_UV];const godot::PackedColorArray colours=actual[godot::Mesh::ARRAY_COLOR];const auto normals=normal_bytes(actual[godot::Mesh::ARRAY_CUSTOM0]);const godot::PackedInt32Array indices=actual[godot::Mesh::ARRAY_INDEX];
    require(p.size()==6&&uv.size()==6&&colours.size()==6&&normals.size()==96&&indices.size()==6,"Merged attribute counts changed");
    for(unsigned part=0;part<2;++part){const auto source=sources[part]->surface_get_arrays(0);const godot::PackedVector3Array sp=source[godot::Mesh::ARRAY_VERTEX];const godot::PackedVector2Array su=source[godot::Mesh::ARRAY_TEX_UV];const godot::PackedColorArray sc=source[godot::Mesh::ARRAY_COLOR];const auto sn=normal_bytes(source[godot::Mesh::ARRAY_CUSTOM0]);for(unsigned i=0;i<3;++i){require(p[part*3+i]==sp[i]&&uv[part*3+i]==su[i]&&colours[part*3+i]==sc[i],"Merged position/UV/colour changed");require(indices[part*3+i]==int(part*3+i),"Merged triangle order changed");}require(std::memcmp(normals.ptr()+part*48,sn.ptr(),48)==0,"CUSTOM0 raw float bits changed");}
    auto hit=cache.acquire(frame,batch);require(hit==merged&&cache.builds()==1&&cache.entries()==1,"Geometry sequence cache miss");cache.trim_unreferenced();require(cache.entries()==1,"Live mesh was evicted");
    // CPU attribute transport must never upload/read back intermediate source
    // meshes. Compare the final GPU arrays with the historical transport.
    unsigned cpu_reads=0;
    TrackMeshBatchCache direct([](std::size_t,std::size_t)->godot::Ref<godot::ArrayMesh>{throw std::runtime_error("CPU merge uploaded a temporary source mesh");},0,
        [&](std::size_t solid,std::size_t){++cpu_reads;return sources.at(solid)->surface_get_arrays(0);});
    auto transported=direct.acquire(frame,batch);const auto transported_arrays=transported->surface_get_arrays(0);
    require(cpu_reads==2&&transported_arrays[godot::Mesh::ARRAY_VERTEX]==actual[godot::Mesh::ARRAY_VERTEX]&&transported_arrays[godot::Mesh::ARRAY_TEX_UV]==actual[godot::Mesh::ARRAY_TEX_UV]&&transported_arrays[godot::Mesh::ARRAY_COLOR]==actual[godot::Mesh::ARRAY_COLOR]&&transported_arrays[godot::Mesh::ARRAY_INDEX]==actual[godot::Mesh::ARRAY_INDEX]&&normal_bytes(transported_arrays[godot::Mesh::ARRAY_CUSTOM0])==normals,"CPU merge changed original GPU attributes");
    const auto bytes=cache.bytes();hit.unref();merged.unref();cache.trim_unreferenced();require(cache.entries()==0&&cache.bytes()==0,"Unreferenced LRU mesh was retained");
    auto again=cache.acquire(frame,batch);require(cache.builds()==2,"Evicted geometry did not rebuild");cache.clear();require(cache.entries()==0&&cache.bytes()==0&&again->get_surface_count()==1,"Clear invalidated externally owned geometry");
    // Different transforms of identical geometry must not alias the cache key.
    frame.commands[0].matrix[0]=frame.commands[0].matrix[5]=frame.commands[0].matrix[10]=frame.commands[0].matrix[15]=1;
    frame.commands[1].matrix=frame.commands[0].matrix;frame.commands[1].matrix[12]=8;
    batch.baked_world=true;
    auto baked=cache.acquire(frame,batch);const godot::PackedVector3Array world=baked->surface_get_arrays(0)[godot::Mesh::ARRAY_VERTEX];
    require(world[3]==godot::Vector3(10,0,0),"World batch changed local positions before GPU matrix evaluation");
    const auto row_bytes=normal_bytes(baked->surface_get_arrays(0)[godot::Mesh::ARRAY_CUSTOM3]);float translation=0;std::memcpy(&translation,row_bytes.ptr()+(3*4+3)*sizeof(float),sizeof(float));require(translation==8,"Native X translation did not reach host Z matrix row");
    require(cache.acquire(frame,batch)==baked,"World transform cache miss");
    frame.commands[1].matrix[12]=9;auto moved=cache.acquire(frame,batch);
    require(moved!=baked,"Different world matrices aliased the mesh cache");
    const godot::PackedVector3Array moved_positions=moved->surface_get_arrays(0)[godot::Mesh::ARRAY_VERTEX];
    require(moved_positions[3]==world[3]&&moved->get_custom_aabb()!=baked->get_custom_aabb(),"World cache retained stale bounds");
    // Native WorkerThreadPool completion order must not affect packed GPU data.
    TrackCommandBatch single0;single0.vertices=3;single0.indices=3;single0.baked_world=true;single0.subranges={{0,0,3,0,3}};
    TrackCommandBatch single1=single0;single1.subranges[0].command=1;
    std::vector<TrackCommandBatch> jobs{single0,single1,single0};
    std::array<godot::Array,2> reference;
    for(unsigned workers:{0u,2u,4u}){
        TrackMeshBatchCache threaded([&](std::size_t solid,std::size_t){return sources.at(solid);});
        threaded.prepare_view(frame,jobs,{true,true,true},workers,71,4);
        const auto stats=threaded.preparation_stats();require(int64_t(stats["jobs"])==2,"Duplicate preparation misses were not deduplicated");
        for(unsigned i=0;i<2;++i){const auto actual=threaded.acquire(frame,jobs[i])->surface_get_arrays(0);
            if(!workers)reference[i]=actual;
            else for(unsigned slot:{unsigned(godot::Mesh::ARRAY_VERTEX),unsigned(godot::Mesh::ARRAY_TEX_UV),unsigned(godot::Mesh::ARRAY_COLOR),unsigned(godot::Mesh::ARRAY_INDEX)})require(actual[slot]==reference[i][slot],"Pool changed packed mesh bytes");
            if(workers)for(unsigned slot=godot::Mesh::ARRAY_CUSTOM0;slot<=godot::Mesh::ARRAY_CUSTOM3;++slot)require(normal_bytes(actual[slot])==normal_bytes(reference[i][slot]),"Pool changed matrix/normal float bytes");
        }
        threaded.finish_view();threaded.prepare_view(frame,jobs,{true,true,true},workers,72,4);require(int64_t(threaded.preparation_stats()["jobs"])==0,"Cached meshes spawned preparation jobs");threaded.clear();
    }
    // Exceptions stay inside native callbacks until the complete group has joined.
    for(unsigned workers:{0u,2u,4u}){
        TrackMeshBatchCache failing([&](std::size_t solid,std::size_t){return sources.at(solid);});
        auto bad=jobs;bad[1].vertices=4;bool rejected=false;
        try{failing.prepare_view(frame,bad,{true,true,true},workers,80,4);}catch(const std::runtime_error&){rejected=true;}
        require(rejected,"Invalid pool source did not propagate after join");
        failing.clear();failing.prepare_view(frame,jobs,{true,true,true},workers,81,4);require(failing.acquire(frame,jobs[0]).is_valid(),"Failed pool preparation poisoned later views");failing.finish_view();
    }
    result["worker_limits_tested"]="0/2/4; deduplication, cache hits, CUSTOM0..3 exact bytes";
    result["ok"]=true;result["bytes"]=int64_t(bytes);result["normal_storage"]=int(sources[0]->surface_get_arrays(0)[godot::Mesh::ARRAY_CUSTOM0].get_type());
 }catch(const std::exception&e){result["ok"]=false;result["error"]=e.what();}return result;}
}
