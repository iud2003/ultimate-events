// A small FFGL host for lifecycle, real shader compilation and readback tests.
#include <winsock2.h>
#include <windows.h>
#include <ffgl/FFGL.h>
#include <glsdk_0_5_2/glload/include/gl_load.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cstring>
using Json=nlohmann::json;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Main=FFMixed(__stdcall*)(FFUInt32,FFMixed,FFInstanceID);
void oscString(std::string& out,const std::string& s){out+=s;out.push_back(0);while(out.size()%4)out.push_back(0);}
struct Sender {
 SOCKET sock; sockaddr_in dest{};
 Sender(int port){WSADATA w;WSAStartup(MAKEWORD(2,2),&w);sock=socket(AF_INET,SOCK_DGRAM,0);DWORD timeout=500;setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));dest.sin_family=AF_INET;dest.sin_addr.s_addr=htonl(INADDR_LOOPBACK);dest.sin_port=htons((u_short)port);}
 ~Sender(){closesocket(sock);WSACleanup();}
 void send(const char* command,Json payload=nullptr){std::string data;oscString(data,std::string("/pulsetext/1/")+command);oscString(data,payload.is_null()?",":",s");if(!payload.is_null())oscString(data,payload.dump());sendto(sock,data.data(),(int)data.size(),0,(sockaddr*)&dest,sizeof(dest));std::this_thread::sleep_for(std::chrono::milliseconds(30));}
 int count(){for(int attempt=0;attempt<4;attempt++){send("ping");char data[4096];int n=recv(sock,data,sizeof(data),0);if(n<=0)continue;size_t p=(strlen(data)+4)&~size_t(3);p=(p+strlen(data+p)+4)&~size_t(3);return Json::parse(std::string(data+p,n-p).c_str())["instances"].get<int>();}throw std::runtime_error("Plugin status reply missing");}
};
int main(int argc,char** argv){
 try {
  require(argc>1,"Pass DLL path and optional effect flag");bool effect=argc>2;
  WNDCLASSA cls{};cls.lpfnWndProc=DefWindowProcA;cls.hInstance=GetModuleHandle(nullptr);cls.lpszClassName="PulseTextTestHost";cls.style=CS_OWNDC;RegisterClassA(&cls);
  HWND window=CreateWindowA(cls.lpszClassName,"PulseText test",WS_OVERLAPPED,0,0,640,360,nullptr,nullptr,cls.hInstance,nullptr);require(window!=nullptr,"CreateWindow failed");
  HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=32;pf.cAlphaBits=8;
  int format=ChoosePixelFormat(dc,&pf);require(format&&SetPixelFormat(dc,format,&pf),"Pixel format failed");HGLRC context=wglCreateContext(dc);require(context&&wglMakeCurrent(dc,context),"WGL context failed");
  require(ogl_LoadFunctions()!=ogl_LOAD_FAILED,"OpenGL functions unavailable");
  std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<" / "<<glGetString(GL_RENDERER)<<"\n";
  HMODULE dll=LoadLibraryA(argv[1]);require(dll!=nullptr,"DLL load failed");auto entry=(Main)GetProcAddress(dll,"plugMain");require(entry!=nullptr,"plugMain export missing");
  auto call=[&](unsigned fn,void* payload=nullptr,void* instance=nullptr){FFMixed value{};value.PointerValue=payload;return entry(fn,value,instance);};
  require(call(FF_INITIALISE_V2).UIntValue==FF_SUCCESS,"FF_INITIALISE failed");
  int w=640,h=360;FFGLViewportStruct viewport{0,0,(GLuint)w,(GLuint)h};glViewport(0,0,w,h);
  void* instance=call(FF_INSTANTIATE_GL,&viewport).PointerValue;require(instance&&instance!=(void*)(uintptr_t)FF_FAIL,"FFGL instantiate/shaders failed");
  GLuint fbo,output,input;glGenFramebuffers(1,&fbo);glGenTextures(1,&output);glBindTexture(GL_TEXTURE_2D,output);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Test FBO incomplete");
  std::vector<unsigned char> blue((size_t)w*h*4,0);for(size_t n=0;n<blue.size();n+=4){blue[n+2]=255;blue[n+3]=255;}
  glGenTextures(1,&input);glBindTexture(GL_TEXTURE_2D,input);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,blue.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  FFGLTextureStruct inputInfo{(FFUInt32)w,(FFUInt32)h,(FFUInt32)w,(FFUInt32)h,input};FFGLTextureStruct* inputs[]={&inputInfo};ProcessOpenGLStruct process{effect?1u:0u,effect?inputs:nullptr,fbo};
  auto render=[&](){
   // A host may leave a different draw framebuffer active while supplying its
   // intended output via HostFBO. The plugin must target HostFBO and restore it.
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
   glEnable(GL_BLEND);glEnable(GL_SCISSOR_TEST);glScissor(0,0,1,1);glActiveTexture(GL_TEXTURE1);glPixelStorei(GL_UNPACK_ALIGNMENT,8);glPixelStorei(GL_UNPACK_ROW_LENGTH,11);
   require(call(FF_PROCESS_OPENGL,&process,instance).UIntValue==FF_SUCCESS,"FF_PROCESS_OPENGL failed");
   GLint active,unpack,row,boundOutput;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&boundOutput);require(boundOutput==0,"Host draw framebuffer binding was not restored");
   require(glIsEnabled(GL_BLEND)&&glIsEnabled(GL_SCISSOR_TEST)&&active==GL_TEXTURE1&&unpack==8&&row==11,"Host GL state was not restored");
   std::vector<unsigned char> pixels((size_t)w*h*4);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());require(glGetError()==GL_NO_ERROR,"OpenGL error during rendering");return pixels;
  };
  const char* portOverride=std::getenv(effect?"PULSETEXT_EFFECT_PORT":"PULSETEXT_SOURCE_PORT");
  Sender sender(portOverride?std::atoi(portOverride):(effect?12001:12000));std::this_thread::sleep_for(std::chrono::milliseconds(100));require(sender.count()==1,"Prototype or wrong instance registered");
  auto initialText=render();
  Json state={{"text",u8"PulseText 日本語 العربية 🎤"},{"size",48},{"colour",{1,1,1,1}},{"timeout",.3}};sender.send("state",state);
  auto pixels=render();require(pixels!=initialText,"Incoming state did not replace the cached default text");
  Json replacement=state;replacement["text"]="UPDATED LIVE";sender.send("state",replacement);auto updatedText=render();require(updatedText!=pixels,"Text-only update did not replace the cached glyph mask");sender.send("state",state);pixels=render();
  size_t changed=0;for(size_t n=0;n<pixels.size();n+=4)if(effect?pixels[n]>0:pixels[n+3]>0)changed++;
  require(changed>100&&changed<(size_t)w*h/2,"Text did not render with expected transparency/compositing");
  sender.send("blackout",{{"enabled",true}});pixels=render();require(effect?pixels==blue:std::all_of(pixels.begin(),pixels.end(),[](unsigned char c){return c==0;}),"Blackout pass-through/transparent output failed");
  sender.send("blackout",{{"enabled",false}});
  state["strobe"]=true;state["strobe_target"]="Whole output";state["strobe_rate"]=.1;state["pulse_width"]=.95;
  sender.send("state",state);sender.send("restart");pixels=render();require(pixels[0]==255&&pixels[1]==255&&pixels[2]==255,"Whole-output strobe did not flash");
  std::this_thread::sleep_for(std::chrono::milliseconds(350));pixels=render();require(pixels[0]==0,"Strobe did not expire");sender.send("heartbeat");pixels=render();require(pixels[0]==0,"Heartbeat unexpectedly rearmed expired strobe");
  void* second=call(FF_INSTANTIATE_GL,&viewport).PointerValue;require(second&&second!=(void*)(uintptr_t)FF_FAIL,"Second instance failed");require(sender.count()==2,"Duplicate IDs were not reported");
  auto beforeDuplicate=pixels;sender.send("blackout",{{"enabled",true}});require(render()==beforeDuplicate,"Duplicate ID accepted a control command");
  SetParameterStruct parameter{};parameter.ParameterNumber=7;float id=(7.f-1)/998;memcpy(&parameter.NewParameterValue.UIntValue,&id,4);require(call(FF_SET_PARAMETER,&parameter,second).UIntValue==FF_SUCCESS,"ID setting failed");require(sender.count()==1,"Instance routing after ID change failed");
  require(call(FF_DEINSTANTIATE_GL,nullptr,second).UIntValue==FF_SUCCESS,"Second instance destruction failed");
  glDisable(GL_SCISSOR_TEST);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);w=320;h=180;viewport.width=w;viewport.height=h;glViewport(0,0,w,h);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,output);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);require(call(FF_RESIZE,&viewport,instance).UIntValue==FF_SUCCESS,"Resize callback failed");render();
  require(call(FF_DEINSTANTIATE_GL,nullptr,instance).UIntValue==FF_SUCCESS,"Instance destruction failed");call(FF_DEINITIALISE);FreeLibrary(dll);
  glDeleteTextures(1,&input);glDeleteTextures(1,&output);glDeleteFramebuffers(1,&fbo);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
  std::cout<<"PASS: text, transparency/overlay, GL state, blackout, watchdog latch, duplicate routing, resize, destruction\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
