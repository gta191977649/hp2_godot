#pragma once
#include "port/render/frame.hpp"
#include "game_core/audio/sound.hpp"
#include <map>
#include <string>
#include <variant>
#include <type_traits>
#include <sstream>
// Diagnostic snapshots only. Production rendering uses typed Frame/Quad directly.
namespace port::runtime {
struct Latin1 { std::string bytes; };
struct Value {
 using Array=std::vector<Value>;
 using Object=std::map<std::string,Value>;
 using IntMap=std::map<std::int64_t,Value>;
 using Pcm=std::shared_ptr<const hp2::port::audio::PcmSound>;
 std::variant<std::monostate,bool,std::int64_t,double,std::string,Latin1,render::Rect,render::Color,Pcm,Array,Object,IntMap> data;
 Value()=default;
 Value(bool v):data(v){}
 template<class T,std::enable_if_t<std::is_integral_v<T>&&!std::is_same_v<T,bool>,int> =0> Value(T v):data(std::int64_t(v)){}
 Value(float v):data(double(v)){} Value(double v):data(v){}
 Value(const char* v):data(std::string(v)){} Value(std::string v):data(std::move(v)){}
 Value(Latin1 v):data(std::move(v)){} Value(render::Rect v):data(v){} Value(render::Color v):data(v){}
 Value(Pcm v):data(std::move(v)){} Value(Array v):data(std::move(v)){}
 Value(Object v):data(std::move(v)){} Value(IntMap v):data(std::move(v)){}
 bool boolean() const { if(auto*p=std::get_if<bool>(&data))return *p;return false; }
 explicit operator bool()const{return boolean();}
 std::int64_t integer() const {if(auto*p=std::get_if<std::int64_t>(&data))return *p;return 0;}
 std::string text()const{if(auto*p=std::get_if<std::string>(&data))return*p;return {};}
};
struct Record:Value::Object {
 Value get(const std::string& key,Value fallback={})const{auto it=find(key);return it==end()?fallback:it->second;}
};
struct Array:Value::Array {void append(Value v){push_back(std::move(v));}};
inline std::string number(std::int64_t value,int base=10){std::ostringstream s;if(base==16)s<<std::hex;s<<value;return s.str();}
} // namespace port::runtime
