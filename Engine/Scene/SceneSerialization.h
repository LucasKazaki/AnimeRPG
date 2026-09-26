#pragma once
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
namespace Astral::Scene {
struct EntityRecord { std::uint32_t id=0; float x=0, y=0, z=0; };
struct SceneSnapshot { std::vector<EntityRecord> entities; };
constexpr std::size_t kMaxSerializedEntities=256;
namespace detail {
inline bool fail(SceneSnapshot& o,std::string* e,const char* m){o.entities.clear();if(e)*e=m;return false;}
inline bool uintValue(std::string_view s,std::uint64_t& v){if(s.empty())return false;auto r=std::from_chars(s.data(),s.data()+s.size(),v);return r.ec==std::errc{}&&r.ptr==s.data()+s.size();}
inline bool floatValue(std::string_view s,float& v){if(s.empty())return false;std::string t(s);std::size_t n=0;try{v=std::stof(t,&n);}catch(...){return false;}return n==t.size()&&std::isfinite(v);}
inline bool fields(std::string_view s,std::string_view f[4]){std::size_t b=0;for(int i=0;i<4;i++){auto p=s.find('|',b);if(i==3){f[i]=s.substr(b);return p==std::string_view::npos;}if(p==std::string_view::npos)return false;f[i]=s.substr(b,p-b);b=p+1;}return false;}
}
inline std::string SerializeCanonical(const SceneSnapshot& s){auto v=s.entities;std::sort(v.begin(),v.end(),[](auto&a,auto&b){return a.id<b.id;});std::ostringstream o;o.imbue(std::locale::classic());o<<"ASTRAL_SCENE_V1\ncount="<<v.size()<<'\n'<<std::fixed<<std::setprecision(9);for(auto&e:v)o<<e.id<<'|'<<e.x<<'|'<<e.y<<'|'<<e.z<<'\n';return o.str();}
inline bool DeserializeCanonical(std::string_view text,SceneSnapshot& o,std::string* e=nullptr){o.entities.clear();if(text.size()>65536)return detail::fail(o,e,"scene payload exceeds bounded size");std::istringstream in{std::string(text)};in.imbue(std::locale::classic());std::string line;if(!std::getline(in,line)||line!="ASTRAL_SCENE_V1")return detail::fail(o,e,"invalid scene header");if(!std::getline(in,line)||line.rfind("count=",0)!=0)return detail::fail(o,e,"invalid scene count");std::uint64_t n=0;if(!detail::uintValue(std::string_view(line).substr(6),n)||n>kMaxSerializedEntities)return detail::fail(o,e,"scene count exceeds bounds");o.entities.reserve((std::size_t)n);std::uint32_t prev=0;bool have=false;for(std::uint64_t i=0;i<n;i++){if(!std::getline(in,line))return detail::fail(o,e,"scene record is truncated");std::string_view f[4];if(!detail::fields(line,f))return detail::fail(o,e,"scene record has invalid fields");std::uint64_t id=0;EntityRecord x;if(!detail::uintValue(f[0],id)||id>UINT32_MAX||!detail::floatValue(f[1],x.x)||!detail::floatValue(f[2],x.y)||!detail::floatValue(f[3],x.z))return detail::fail(o,e,"scene record contains invalid values");x.id=(std::uint32_t)id;if(have&&x.id<=prev)return detail::fail(o,e,"scene records are not canonical");prev=x.id;have=true;o.entities.push_back(x);}if(std::getline(in,line))return detail::fail(o,e,"scene payload contains trailing data");return true;}
}