#pragma once
// Port-only main-thread staging. Never applies a material or advances a frame.
#include <array>
#include <cstdint>
#include <map>
#include <memory>
namespace port::godot_bridge {
template<class Name,class Value,std::size_t Capacity=4096>
class ParameterStagingRegistry {
public:
    struct Snapshot {
        std::uint64_t material=0,shader=0,generation=0,version=0;
        std::array<std::uint64_t,8> parameter_versions{};
        std::array<Name,8> names;
        std::array<Value,8> values;
        std::array<bool,8> valid{};
    };
    struct Statistics {std::uint64_t registrations=0,published=0,hits=0,misses=0,invalidations=0,evictions=0,snapshot_reads=0;std::size_t entries=0;};
    using Owner=std::shared_ptr<Snapshot>;
private:
    struct Entry {std::weak_ptr<Snapshot> value;std::uint64_t serial;};
    std::map<std::uint64_t,Entry> entries_;
    Statistics stats_;
    std::uint64_t serial_=0,generation_=0;
public:
    Owner create(std::uint64_t material,std::uint64_t shader,const std::array<Name,8>& names){
        static_assert(Capacity>0,"Staging registry requires a positive bound");
        if(entries_.size()>=Capacity&&!entries_.count(material)){
            for(auto it=entries_.begin();it!=entries_.end();)if(it->second.value.expired())it=entries_.erase(it);else ++it;
            if(entries_.size()>=Capacity){
                auto oldest=entries_.begin();for(auto it=entries_.begin();it!=entries_.end();++it)if(it->second.serial<oldest->second.serial)oldest=it;
                entries_.erase(oldest);++stats_.evictions;
            }
        }
        auto value=std::make_shared<Snapshot>();value->generation=++generation_;value->material=material;value->shader=shader;value->names=names;
        entries_[material]={value,++serial_};++stats_.registrations;return value;
    }
    void publish(const Owner& owner,std::size_t index,const Value& value){
        if(!owner||index>=owner->values.size())return;
        if(!owner->valid[index]||!(owner->values[index]==value)){owner->values[index]=value;owner->parameter_versions[index]=++owner->version;}
        owner->valid[index]=true;++stats_.published;
    }
    // Detached copy at the original consumer point: no live registry crosses workers.
    bool read_snapshot(std::uint64_t material,std::uint64_t shader,Snapshot& value){
        auto found=entries_.find(material);
        if(found==entries_.end()){++stats_.misses;return false;}
        auto owner=found->second.value.lock();
        if(!owner){entries_.erase(found);++stats_.misses;return false;}
        if(owner->shader!=shader){++stats_.misses;return false;}
        value=*owner;found->second.serial=++serial_;++stats_.snapshot_reads;return true;
    }
    bool lookup(std::uint64_t material,std::uint64_t shader,const Name& name,Value& value){
        Snapshot snapshot;if(!read_snapshot(material,shader,snapshot))return false;
        for(std::size_t i=0;i<snapshot.names.size();++i)if(snapshot.names[i]==name&&snapshot.valid[i]){value=snapshot.values[i];++stats_.hits;return true;}
        ++stats_.misses;return false;
    }
    void invalidate(std::uint64_t material){
        auto found=entries_.find(material);if(found==entries_.end())return;
        if(auto owner=found->second.value.lock()){owner->valid.fill(false);++owner->version;++stats_.invalidations;}
        else entries_.erase(found);
    }
    Statistics stats()const{auto result=stats_;result.entries=entries_.size();return result;}
};
}
