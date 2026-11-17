#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
using Json = nlohmann::json;
inline double nowSeconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline float number(const Json& j, const char* key, float fallback, float lo, float hi) {
 auto it=j.find(key); if(it==j.end() || !it->is_number()) return fallback;
 float v=it->get<float>(); return std::isfinite(v)?std::clamp(v,lo,hi):fallback;
}
inline bool flag(const Json& j,const char* k,bool def=false) { auto i=j.find(k);return i!=j.end()&&i->is_boolean()?i->get<bool>():def; }
inline std::string word(const Json& j,const char* k,const char* def="") {auto i=j.find(k);return i!=j.end()&&i->is_string()?i->get<std::string>():def;}
struct State {
 std::mutex mutex;
 Json settings=Json::object();
 float bass=0,mid=0,treble=0,flash=0;
 double heartbeat=0,epoch=nowSeconds();
 bool stopped=true,blackout=false;
 int id=1;
 std::string renderedText,renderError;
 unsigned long long frames=0;
 double lastRender=0;
};
// Call with State::mutex held. Connection loss latches strobe OFF until rearmed.
inline void expireStrobe(State& s,double now) {
 if(now-s.heartbeat>number(s.settings,"timeout",2,.3f,10)){
  s.stopped=true;s.settings["strobe"]=false;
 }
}
