#include "track_mesh_batches.hpp"
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/variant/array.hpp>
#include <map>
#include <atomic>
#include <chrono>
#include <exception>
#include <cstring>
namespace port::godot_bridge {
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
using Key=std::pair<std::vector<std::pair<std::size_t,std::size_t>>,std::vector<std::array<std::uint32_t,16>>>;
Key key_of(const port::track::Frame& frame,const TrackCommandBatch& batch){
 if(batch.subranges.empty())throw std::invalid_argument("Empty track batch");
 Key key;auto expected=batch.subranges.front().command;
 for(const auto& r:batch.subranges){if(r.command!=expected++)throw std::invalid_argument("Track batch commands are not contiguous");const auto& c=frame.commands.at(r.command);key.first.emplace_back(c.solid,c.surface);if(batch.baked_world){std::array<std::uint32_t,16> bits;std::memcpy(bits.data(),c.matrix.data(),64);key.second.push_back(bits);}}return key;
}
std::shared_ptr<const port::render::MeshAttributes> snapshot_arrays(const godot::Array& a,std::uint64_t format){
 if(a.size()!=godot::Mesh::ARRAY_MAX)throw std::runtime_error("Track batch source array size changed");
 constexpr std::uint64_t allowed=godot::Mesh::ARRAY_FORMAT_VERTEX|godot::Mesh::ARRAY_FORMAT_TEX_UV|godot::Mesh::ARRAY_FORMAT_COLOR|godot::Mesh::ARRAY_FORMAT_CUSTOM0|godot::Mesh::ARRAY_FORMAT_INDEX;
 if(((format>>godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT)&godot::Mesh::ARRAY_FORMAT_CUSTOM_MASK)!=godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT||(format&((1u<<godot::Mesh::ARRAY_MAX)-1u)&~allowed))throw std::runtime_error("Track batch unsupported source format");
 for(unsigned s=0;s<godot::Mesh::ARRAY_MAX;++s)if(!(allowed&(1u<<s))&&a[s].get_type()!=godot::Variant::NIL)throw std::runtime_error("Track batch unsupported CPU attribute");
 auto out=std::make_shared<port::render::MeshAttributes>();
 const godot::PackedVector3Array p=a[godot::Mesh::ARRAY_VERTEX];const godot::PackedVector2Array uv=a[godot::Mesh::ARRAY_TEX_UV];const godot::PackedColorArray c=a[godot::Mesh::ARRAY_COLOR];const godot::PackedInt32Array indices=a[godot::Mesh::ARRAY_INDEX];
 godot::PackedFloat32Array n;const godot::Variant custom=a[godot::Mesh::ARRAY_CUSTOM0];
 if(custom.get_type()==godot::Variant::PACKED_BYTE_ARRAY){const godot::PackedByteArray raw=custom;if(raw.size()%sizeof(float))throw std::runtime_error("Track batch truncated normal data");n.resize(raw.size()/sizeof(float));std::memcpy(n.ptrw(),raw.ptr(),raw.size());}
 else if(custom.get_type()==godot::Variant::PACKED_FLOAT32_ARRAY)n=custom;else throw std::runtime_error("Track batch unsupported normal data");
 if(uv.size()!=p.size()||c.size()!=p.size()||n.size()!=p.size()*4)throw std::runtime_error("Track batch source attributes changed");
 for(int64_t i=0;i<p.size();++i){out->positions.push_back({{p[i].x,p[i].y,p[i].z}});out->uv.push_back({{uv[i].x,uv[i].y}});out->colours.push_back({{c[i].r,c[i].g,c[i].b,c[i].a}});out->normals.push_back({{n[i*4],n[i*4+1],n[i*4+2],n[i*4+3]}});}
 for(int64_t i=0;i<indices.size();++i)out->indices.push_back(indices[i]);return out;
}
godot::Array pack(const port::render::MeshPrepareOutput& out){
 godot::PackedVector3Array p;godot::PackedVector2Array uv;godot::PackedColorArray c;godot::PackedFloat32Array n;godot::PackedInt32Array indices;
 p.resize(out.positions.size());uv.resize(out.uv.size());c.resize(out.colours.size());n.resize(out.normals.size()*4);indices.resize(out.indices.size());
 static_assert(sizeof(godot::Vector3)==sizeof(std::array<float,3>)&&sizeof(godot::Vector2)==sizeof(std::array<float,2>)&&sizeof(godot::Color)==sizeof(std::array<float,4>),"Pinned single-precision packed ABI required");
 if(!out.positions.empty())std::memcpy(p.ptrw(),out.positions.data(),out.positions.size()*sizeof(out.positions[0]));
 if(!out.uv.empty())std::memcpy(uv.ptrw(),out.uv.data(),out.uv.size()*sizeof(out.uv[0]));
 if(!out.colours.empty())std::memcpy(c.ptrw(),out.colours.data(),out.colours.size()*sizeof(out.colours[0]));
 if(!out.normals.empty())std::memcpy(n.ptrw(),out.normals.data(),out.normals.size()*sizeof(out.normals[0]));
 if(!out.indices.empty())std::memcpy(indices.ptrw(),out.indices.data(),out.indices.size()*sizeof(out.indices[0]));
 godot::Array a;a.resize(godot::Mesh::ARRAY_MAX);a[godot::Mesh::ARRAY_VERTEX]=p;a[godot::Mesh::ARRAY_TEX_UV]=uv;a[godot::Mesh::ARRAY_COLOR]=c;a[godot::Mesh::ARRAY_CUSTOM0]=n;a[godot::Mesh::ARRAY_INDEX]=indices;
 for(unsigned r=0;r<3;++r)if(!out.rows[r].empty()){godot::PackedFloat32Array row;row.resize(out.rows[r].size()*4);std::memcpy(row.ptrw(),out.rows[r].data(),out.rows[r].size()*sizeof(out.rows[r][0]));a[godot::Mesh::ARRAY_CUSTOM1+r]=row;}return a;
}
using Job=port::render::MeshPreparationJob;
using Work=port::render::PreparationWork;
}
struct TrackMeshBatchCache::Impl {
 struct Entry {godot::Ref<godot::ArrayMesh> mesh;std::size_t bytes=0;std::uint64_t used=0;};
 MeshLookup lookup;ArrayLookup arrays;CpuLookup cpu;SourceReady ready;std::map<Key,Entry> cache;std::map<Key,port::render::MeshPrepareOutput> prepared;std::vector<port::render::SubmitMathOutput> math;
 std::size_t limit,total=0,builds=0,uploaded=0;std::uint64_t clock=0,generation=0;godot::Dictionary stats;double packed=0,upload=0,commit=0,late=0,source_upload=0;
 Impl(MeshLookup source,std::size_t maximum):lookup(std::move(source)),limit(maximum){if(!lookup)throw std::invalid_argument("Track mesh batches require source lookup");}
 port::render::MeshPrepareInput snapshot(const port::track::Frame& frame,const TrackCommandBatch& batch){
  port::render::MeshPrepareInput in;in.vertices=batch.vertices;in.indices=batch.indices;in.baked_world=batch.baked_world;
  for(const auto& r:batch.subranges){const auto& c=frame.commands.at(r.command);port::render::MeshPreparePart p;p.matrix=c.matrix;p.first_vertex=r.first_vertex;p.vertices=r.vertex_count;p.first_index=r.first_index;p.indices=r.index_count;
   if(cpu)p.decoded=cpu(c.solid,c.surface);
   else{godot::Array a;auto format=std::uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT;
    if(arrays)a=arrays(c.solid,c.surface);else{const auto mesh=lookup(c.solid,c.surface);if(mesh.is_null()||mesh->get_surface_count()!=1||mesh->surface_get_primitive_type(0)!=godot::Mesh::PRIMITIVE_TRIANGLES)throw std::runtime_error("Track batch requires one triangle surface");a=mesh->surface_get_arrays(0);format=mesh->surface_get_format(0);}p.attributes=snapshot_arrays(a,format);}
   in.parts.push_back(std::move(p));}return in;
 }
};
TrackMeshBatchCache::TrackMeshBatchCache(MeshLookup source,std::size_t bytes,ArrayLookup arrays):impl_(std::make_unique<Impl>(std::move(source),bytes)){impl_->arrays=std::move(arrays);}
TrackMeshBatchCache::~TrackMeshBatchCache()=default;
void TrackMeshBatchCache::set_cpu_source(CpuLookup cpu,SourceReady ready){impl_->cpu=std::move(cpu);impl_->ready=std::move(ready);}
void TrackMeshBatchCache::prepare_view(const port::track::Frame& frame,const std::vector<TrackCommandBatch>& batches,const std::vector<bool>& requested,unsigned workers,std::uint64_t source_frame,unsigned view,std::uint64_t scene_generation,std::vector<port::render::SubmitMathInput> math){
 if(workers!=0&&workers!=2&&workers!=4)throw std::invalid_argument("Render preparation workers must be 0, 2 or 4");
 if(requested.size()!=batches.size())throw std::invalid_argument("Render preparation request count changed");
 impl_->prepared.clear();impl_->math.clear();impl_->stats.clear();impl_->packed=impl_->upload=impl_->commit=impl_->late=impl_->source_upload=0;
 const auto begin=Clock::now();Work work;std::map<Key,std::size_t> slots;std::size_t hits=0,reused=0;
 for(std::size_t i=0;i<batches.size();++i){if(!requested[i]){++reused;continue;}auto key=key_of(frame,batches[i]);
  if(key.first.size()==1&&key.second.empty()){if(!impl_->cpu)continue;const auto id=key.first.front();if(impl_->ready&&impl_->ready(id.first,id.second)){++hits;continue;}}
  else if(impl_->cache.count(key)){++hits;continue;}
  if(slots.count(key))continue;slots.emplace(std::move(key),work.jobs.size());Job j;j.input=impl_->snapshot(frame,batches[i]);work.jobs.push_back(std::move(j));
 }
 work.configure_math(std::move(math),workers);const auto snapshot=ms(begin);const auto prepared_begin=Clock::now();double dispatch=0,wait=0;
 if(workers&&work.tasks()>1){auto* pool=godot::WorkerThreadPool::get_singleton();if(!pool)throw std::runtime_error("Godot WorkerThreadPool unavailable");
  if(work.tasks()>std::size_t(std::numeric_limits<int>::max()))throw std::length_error("Too many mesh preparation jobs");
  const auto t=Clock::now();const auto group=pool->add_native_group_task(&Work::run,&work,int(work.tasks()),int(std::min<std::size_t>(workers,work.tasks())),true,"HP2 CPU mesh preparation");dispatch=ms(t);
  if(group==godot::WorkerThreadPool::INVALID_TASK_ID)throw std::runtime_error("Godot CPU preparation registration failed");const auto joined=Clock::now();pool->wait_for_group_task_completion(group);wait=ms(joined);
 }else for(std::size_t i=0;i<work.tasks();++i)Work::run(&work,std::uint32_t(i));
 const auto wall=ms(prepared_begin);double sum=0;std::size_t bytes=0;
 work.rethrow_errors();
 for(double duration:work.math_durations)sum+=duration;
 impl_->math=std::move(work.math_outputs);
 for(const auto& slot:slots){auto& j=work.jobs[slot.second];sum+=j.duration;bytes+=j.output.positions.size()*sizeof(float)*(j.input.baked_world?25:13)+j.output.indices.size()*sizeof(std::int32_t);impl_->prepared.emplace(slot.first,std::move(j.output));}
 auto& s=impl_->stats;s["source_frame"]=int64_t(source_frame);s["view"]=view;s["scene_generation"]=int64_t(scene_generation);s["cache_generation"]=int64_t(impl_->generation);s["workers_requested"]=workers;s["peak_concurrent_jobs"]=work.peak.load();s["jobs"]=int64_t(work.tasks());s["mesh_jobs"]=int64_t(work.jobs.size());s["submit_math_entries"]=int64_t(impl_->math.size());s["submit_math_tasks"]=int64_t(work.math_errors.size());std::size_t inverse_hits=0,fallbacks=0;for(const auto& value:impl_->math){inverse_hits+=value.inverse_reused;fallbacks+=!value.supported;}s["inverse_cache_hits"]=int64_t(inverse_hits);s["submit_math_fallbacks"]=int64_t(fallbacks);s["cache_hits"]=int64_t(hits);s["draw_reuses"]=int64_t(reused);s["snapshot_ms"]=snapshot;s["dispatch_ms"]=dispatch;s["prepare_wall_ms"]=wall;s["job_sum_ms"]=sum;s["join_wait_ms"]=wait;s["payload_bytes"]=int64_t(bytes);
}
godot::Array TrackMeshBatchCache::source_arrays(std::size_t solid,std::size_t surface){const auto found=impl_->prepared.find(Key{{{solid,surface}}, {}});if(found==impl_->prepared.end())return {};const auto t=Clock::now();auto a=pack(found->second);impl_->packed+=ms(t);return a;}
godot::Ref<godot::ArrayMesh> TrackMeshBatchCache::acquire(const port::track::Frame& frame,const TrackCommandBatch& batch){
 auto key=key_of(frame,batch);if(key.first.size()==1&&!batch.baked_world)return impl_->lookup(key.first[0].first,key.first[0].second);
 const auto found=impl_->cache.find(key);if(found!=impl_->cache.end()){found->second.used=++impl_->clock;return found->second.mesh;}
 port::render::MeshPrepareOutput fallback;const auto prepared=impl_->prepared.find(key);if(prepared==impl_->prepared.end()){const auto t=Clock::now();fallback=port::render::prepare_mesh(impl_->snapshot(frame,batch));impl_->late+=ms(t);}const auto& out=prepared==impl_->prepared.end()?fallback:prepared->second;
 const auto packing=Clock::now();const auto a=pack(out);impl_->packed+=ms(packing);
 std::uint64_t format=std::uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT;if(batch.baked_world)for(unsigned i=0;i<3;++i)format|=std::uint64_t(godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT)<<(godot::Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT+i*3);
 const auto upload=Clock::now();godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,a,{}, {},format);if(batch.baked_world)mesh->set_custom_aabb(godot::AABB({out.bounds_position[0],out.bounds_position[1],out.bounds_position[2]},{out.bounds_size[0],out.bounds_size[1],out.bounds_size[2]}));impl_->upload+=ms(upload);
 if(mesh->get_surface_count()!=1)throw std::runtime_error("Merged track mesh upload failed");const auto commit=Clock::now();const std::size_t bytes=batch.vertices*sizeof(float)*(batch.baked_world?25:13)+batch.indices*sizeof(std::int32_t);impl_->cache.emplace(std::move(key),Impl::Entry{mesh,bytes,++impl_->clock});impl_->total+=bytes;impl_->uploaded+=bytes;++impl_->builds;trim_unreferenced();impl_->commit+=ms(commit);return mesh;
}
void TrackMeshBatchCache::note_source_upload(double wall_ms){impl_->source_upload+=wall_ms;}
const port::render::SubmitMathOutput& TrackMeshBatchCache::submit_math(std::size_t index)const{return impl_->math.at(index);}
void TrackMeshBatchCache::finish_view(){impl_->prepared.clear();impl_->math.clear();}
godot::Dictionary TrackMeshBatchCache::preparation_stats()const{auto s=impl_->stats.duplicate();s["packed_conversion_ms"]=impl_->packed;s["late_inline_prepare_ms"]=impl_->late;s["source_upload_ms"]=impl_->source_upload;s["batch_upload_ms"]=impl_->upload;s["batch_cache_commit_ms"]=impl_->commit;s["timing_scope"]="CPU wall; job_sum_ms overlaps prepare_wall_ms/join_wait_ms; batch upload excludes singleton source uploads";return s;}
void TrackMeshBatchCache::clear(){finish_view();++impl_->generation;impl_->cache.clear();impl_->total=0;impl_->clock=0;impl_->builds=0;impl_->uploaded=0;}
void TrackMeshBatchCache::trim_unreferenced(){while(impl_->total>impl_->limit){auto oldest=impl_->cache.end();for(auto it=impl_->cache.begin();it!=impl_->cache.end();++it)if(it->second.mesh->get_reference_count()==1&&(oldest==impl_->cache.end()||it->second.used<oldest->second.used))oldest=it;if(oldest==impl_->cache.end())break;impl_->total-=oldest->second.bytes;impl_->cache.erase(oldest);}}
std::size_t TrackMeshBatchCache::bytes()const{return impl_->total;}
std::size_t TrackMeshBatchCache::entries()const{return impl_->cache.size();}
std::size_t TrackMeshBatchCache::builds()const{return impl_->builds;}
std::size_t TrackMeshBatchCache::uploaded_payload_bytes()const{return impl_->uploaded;}
}
