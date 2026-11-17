#include <winsock2.h>
#include <ws2tcpip.h>
#include "Receiver.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace {
std::string readString(const char* data,size_t size,size_t& pos) {
 if(pos>=size)throw std::runtime_error("short OSC");
 size_t end=pos;while(end<size&&data[end])++end;
 if(end==size)throw std::runtime_error("unterminated OSC");
 std::string s(data+pos,end-pos);pos=(end+4)&~size_t(3);
 if(pos>size)throw std::runtime_error("bad padding");return s;
}
void append(std::string& out,const std::string& value){out+=value;out.push_back(0);while(out.size()%4)out.push_back(0);}
}
Receiver::Receiver(int p):port(p) {
#ifdef PULSE_EFFECT
 const char* env=std::getenv("PULSETEXT_EFFECT_PORT");
#else
 const char* env=std::getenv("PULSETEXT_SOURCE_PORT");
#endif
 if(env){int n=std::atoi(env);if(n>=1024&&n<=65535)port=n;}
 thread=std::thread(&Receiver::run,this);
}
Receiver::~Receiver(){running=false;if(thread.joinable())thread.join();}
void Receiver::add(const std::shared_ptr<State>& s){std::lock_guard<std::mutex> l(mutex);instances.push_back(s);}
void Receiver::remove(const std::shared_ptr<State>& s){std::lock_guard<std::mutex> l(mutex);instances.erase(std::remove_if(instances.begin(),instances.end(),[&](const std::weak_ptr<State>& w){auto p=w.lock();return !p||p==s;}),instances.end());}
void Receiver::run(){
 WSADATA w; if(WSAStartup(MAKEWORD(2,2),&w))return;
 SOCKET sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
 if(sock==INVALID_SOCKET){WSACleanup();return;}
 BOOL exclusive=TRUE;setsockopt(sock,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive));
 DWORD timeout=100;setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
 sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_ANY);local.sin_port=htons((u_short)port);
 if(bind(sock,(sockaddr*)&local,sizeof(local))==SOCKET_ERROR){OutputDebugStringA("PulseText: OSC port unavailable; change PULSETEXT_*_PORT and restart Arena\n");closesocket(sock);WSACleanup();return;}
 char data[49152];
 while(running){
  sockaddr_in peer{};int peerLen=sizeof(peer);int n=recvfrom(sock,data,sizeof(data),0,(sockaddr*)&peer,&peerLen);if(n<=0)continue;
  try {
   size_t pos=0;auto address=readString(data,n,pos);auto types=readString(data,n,pos);
   const std::string prefix="/pulsetext/";if(address.rfind(prefix,0)!=0)continue;
   size_t slash=address.find('/',prefix.size());if(slash==std::string::npos)continue;
   auto idText=address.substr(prefix.size(),slash-prefix.size());if(idText.empty()||idText.find_first_not_of("0123456789")!=std::string::npos)continue;
   int id=std::stoi(idText);if(id<1||id>999)continue;
   auto command=address.substr(slash+1);Json payload;
   if(types==",s")payload=Json::parse(readString(data,n,pos));else if(types!=",")continue;
   std::vector<std::shared_ptr<State>> targets;
   {std::lock_guard<std::mutex> lock(mutex);for(auto it=instances.begin();it!=instances.end();){if(auto s=it->lock()){targets.push_back(s);++it;}else it=instances.erase(it);}}
   int matches=0;for(auto& s:targets){std::lock_guard<std::mutex> l(s->mutex);if(s->id==id)++matches;}
   if(command=="ping"){
    Json status={{"id",id},{"instances",matches},{"port",port},{"version","1.0"}};
    if(matches==1)for(auto& s:targets){std::lock_guard<std::mutex> l(s->mutex);if(s->id==id){status["received_text"]=word(s->settings,"text","PulseText");status["rendered_text"]=s->renderedText;status["render_error"]=s->renderError;status["frames"]=s->frames;status["render_age"]=s->lastRender>0?nowSeconds()-s->lastRender:-1;}}
    std::string reply;append(reply,"/pulsetext/status");append(reply,",s");append(reply,status.dump());
    sendto(sock,reply.data(),(int)reply.size(),0,(sockaddr*)&peer,peerLen);continue;
   }
   // Duplicate IDs are rejected instead of silently driving several clips.
   if(matches!=1)continue;
   for(auto& s:targets){std::lock_guard<std::mutex> l(s->mutex);if(s->id!=id)continue;
    if(command=="state"&&payload.is_object()){s->settings=payload;s->heartbeat=nowSeconds();s->stopped=!flag(payload,"strobe");s->blackout=flag(payload,"blackout");}
    else if(command=="audio"&&payload.is_object()){s->bass=number(payload,"bass",0,0,1);s->mid=number(payload,"mid",0,0,1);s->treble=number(payload,"treble",0,0,1);s->flash=number(payload,"flash",0,0,1);s->heartbeat=nowSeconds();}
    else if(command=="heartbeat")s->heartbeat=nowSeconds();
    else if(command=="restart")s->epoch=nowSeconds();
    else if(command=="stop"){s->stopped=true;s->settings["strobe"]=false;}
    else if(command=="blackout"&&payload.is_object())s->blackout=flag(payload,"enabled");
   }
  }catch(const std::exception&){/* Malformed/untrusted datagrams never escape the worker. */}
 }
 closesocket(sock);WSACleanup();
}
std::shared_ptr<Receiver> acquireReceiver(){
 static std::mutex creation;
 static std::weak_ptr<Receiver> weak;
 std::lock_guard<std::mutex> lock(creation);
 auto r=weak.lock();if(r)return r;
#ifdef PULSE_EFFECT
 r=std::make_shared<Receiver>(12001);
#else
 r=std::make_shared<Receiver>(12000);
#endif
 weak=r;return r;
}
