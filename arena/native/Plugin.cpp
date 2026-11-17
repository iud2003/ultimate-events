#include "Receiver.h"
#include "Text.h"
#include <FFGLSDK.h>
#include <array>
#include <cstring>
using namespace ffglex;
namespace {
const char* vertex=R"(#version 410 core
layout(location=0) in vec4 vPosition;layout(location=1) in vec2 vUV;
out vec2 uv;void main(){gl_Position=vPosition;uv=vUV;})";
const char* fragment=R"(#version 410 core
in vec2 uv;out vec4 fragColor;
uniform sampler2D glyphs,video;
uniform vec2 resolution,maxUV,position;
uniform vec4 colour,background;
uniform float elapsed,speed,intensity,opacity,scale,rotation,outline,glow,audioScale,audioGlow,brightness,audioShake,flash;
uniform int animation,effect,blackout,flashMode;
float maskAt(vec2 p){if(any(lessThan(p,vec2(0)))||any(greaterThan(p,vec2(1))))return 0.;return texture(glyphs,vec2(p.x,1.-p.y)).a;}
void main(){
 vec4 base=vec4(0);if(effect==1)base=texture(video,uv*maxUV);
 if(blackout==1){fragColor=base;return;}
 float time=elapsed*speed;float fade=1.;float zoom=scale*(1.+audioScale);vec2 offset=position;
 if(animation==1)offset.x+=1.-mod(time*.2,2.);
 if(animation==2)offset.x+=mod(time*.2,2.)-1.;
 if(animation==3)offset.y+=mod(time*.2,2.)-1.;
 if(animation==4)offset.y+=1.-mod(time*.2,2.);
 if(animation==5)fade=(sin(time*3.14159265)*.5+.5);
 if(animation==6)zoom*=1.+sin(time*6.2831853)*.2*intensity;
 if(animation==7)offset.y+=abs(sin(time*3.14159265))*.3*intensity;
 if(animation==9)offset+=vec2(sin(time*71.),cos(time*83.))*.025*intensity;
 offset+=vec2(sin(elapsed*91.),cos(elapsed*113.))*.04*audioShake;
 vec2 p=(uv-.5-offset)*resolution;float c=cos(rotation),s=sin(rotation);
 p=mat2(c,-s,s,c)*p/max(.01,zoom);p=p/resolution+.5;
 float a=maskAt(p),d=a,g=0.;
 for(int k=0;k<16;k++){float angle=float(k)*6.2831853/16.;vec2 dir=vec2(cos(angle),sin(angle))/resolution;d=max(d,maskAt(p+dir*outline));g+=maskAt(p+dir*(4.+12.*(glow+audioGlow)))/16.;}
 float alpha=clamp((d+g*(glow+audioGlow))*colour.a*opacity*fade,0.,1.);
 vec3 rgb=colour.rgb*(1.+brightness);
 if(flashMode==0){rgb=mix(rgb,vec3(1),flash);alpha=max(alpha,a*flash);}
 vec4 bg=background; if(flashMode==1)bg=mix(bg,vec4(1),flash);
 vec4 layer=vec4(rgb*alpha+bg.rgb*bg.a*(1.-alpha),alpha+bg.a*(1.-alpha));
 vec4 result=layer+base*(1.-layer.a);
 if(flashMode==2)result=mix(result,vec4(1),flash);
 fragColor=result;
})";
// Save every piece of GL state touched here, including SDK quad initialization.
struct Guard {
 GLint program,vao,array,active,tex[2],sampler[2],unpack,pbo,rowLength,skipRows,skipPixels,drawFbo;GLboolean blend,depth,scissor,cull,stencil,discard,mask[4];
 Guard(){glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&array);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&pbo);
 glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);
 for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&tex[i]);glGetIntegeri_v(GL_SAMPLER_BINDING,i,&sampler[i]);glBindSampler(i,0);}glGetBooleanv(GL_COLOR_WRITEMASK,mask);
 blend=glIsEnabled(GL_BLEND);depth=glIsEnabled(GL_DEPTH_TEST);scissor=glIsEnabled(GL_SCISSOR_TEST);cull=glIsEnabled(GL_CULL_FACE);stencil=glIsEnabled(GL_STENCIL_TEST);discard=glIsEnabled(GL_RASTERIZER_DISCARD);
 glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skipRows);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skipPixels);
 glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glDisable(GL_RASTERIZER_DISCARD);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);}
 ~Guard(){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,array);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
 for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tex[i]);glBindSampler(i,sampler[i]);}glActiveTexture(active);
 glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);glPixelStorei(GL_UNPACK_SKIP_ROWS,skipRows);glPixelStorei(GL_UNPACK_SKIP_PIXELS,skipPixels);
 set(GL_BLEND,blend);set(GL_DEPTH_TEST,depth);set(GL_SCISSOR_TEST,scissor);set(GL_CULL_FACE,cull);set(GL_STENCIL_TEST,stencil);set(GL_RASTERIZER_DISCARD,discard);glColorMask(mask[0],mask[1],mask[2],mask[3]);}
 void set(GLenum name,bool on){if(on)glEnable(name);else glDisable(name);}
};
}
class PulseText:public CFFGLPlugin {
 std::shared_ptr<State> state=std::make_shared<State>();
 std::shared_ptr<Receiver> network;
 FFGLShader shader;FFGLScreenQuad quad;GLuint texture=0;
 std::array<float,9> params{1,.1f,.5f,1,1,.2f,1,0,0};
 std::mutex parameterMutex;
 std::string cached;int width=0,height=0,typeFrame=-1;bool failed=false;
public:
 PulseText(){
#ifdef PULSE_EFFECT
 SetMinInputs(1);SetMaxInputs(1);
#else
 SetMinInputs(0);SetMaxInputs(0);
#endif
 const char* names[]={"Opacity","Anim speed","Anim intensity","Audio amount","Strobe enable","Strobe rate","Strobe intensity","Control ID","Blackout"};
 for(unsigned i=0;i<9;i++)SetParamInfof(i,names[i],FF_TYPE_STANDARD);
 }
 FFResult SetFloatParameter(unsigned i,float v) override {if(i>=params.size()||!std::isfinite(v))return FF_FAIL;v=std::clamp(v,0.f,1.f);{std::lock_guard<std::mutex> l(parameterMutex);params[i]=v;}if(i==7){std::lock_guard<std::mutex> l(state->mutex);int id=1+(int)std::round(v*998);if(id!=state->id){state->id=id;state->stopped=true;state->settings["strobe"]=false;}}return FF_SUCCESS;}
 float GetFloatParameter(unsigned i) override {std::lock_guard<std::mutex> l(parameterMutex);return i<params.size()?params[i]:0;}
 char* GetParameterDisplay(unsigned i) override {if(i==7){static thread_local char out[16];snprintf(out,sizeof(out),"%d",1+(int)std::round(GetFloatParameter(7)*998));return out;}return CFFGLPlugin::GetParameterDisplay(i);}
 FFResult InitGL(const FFGLViewportStruct* vp) override {Guard guard;if(!shader.Compile(vertex,fragment)||!quad.Initialise()){DeInitGL();return FF_FAIL;}glGenTextures(1,&texture);cached.clear();width=height=0;network=acquireReceiver();network->add(state);return CFFGLPlugin::InitGL(vp);}
 FFResult DeInitGL() override {Guard guard;if(network){network->remove(state);network.reset();}if(texture)glDeleteTextures(1,&texture);texture=0;shader.FreeGLResources();quad.Release();return FF_SUCCESS;}
 FFResult ProcessOpenGL(ProcessOpenGLStruct* input) override {
  Guard guard;
  if(!input)return FF_FAIL;
  // Arena supplies the destination explicitly; do not depend on a previously
  // bound preview/thumbnail framebuffer being the live output destination.
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,input->HostFBO);
  if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){std::lock_guard<std::mutex> l(state->mutex);state->renderError="Host output framebuffer is incomplete";return FF_FAIL;}
#ifdef PULSE_EFFECT
  if(!input||input->numInputTextures<1||!input->inputTextures[0])return FF_FAIL;
#endif
  Json j;float bass,mid,treble,trigger;double elapsed,last;bool stopped,black;
  {std::lock_guard<std::mutex> l(state->mutex);expireStrobe(*state,nowSeconds());j=state->settings;bass=state->bass;mid=state->mid;treble=state->treble;trigger=state->flash;elapsed=nowSeconds()-state->epoch;last=state->heartbeat;stopped=state->stopped;black=state->blackout;}
  std::array<float,9> par;{std::lock_guard<std::mutex> l(parameterMutex);par=params;}
  GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);int w=viewport[2],h=viewport[3];if(w<1||h<1||w>8192||h>8192)return FF_FAIL;
  // Audio and strobe settings do not cause expensive text rasterization.
  Json textKeys;for(auto key:{"text","font","size","bold","italic","horizontal","vertical","line_spacing","letter_spacing"})if(j.contains(key))textKeys[key]=j[key];
  std::string key=textKeys.dump();int frame=word(j,"animation")=="Typewriter"?(int)(elapsed*number(j,"speed",1,0,10)*par[1]*10*12):-1;
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
  if(key!=cached||w!=width||h!=height||frame!=typeFrame){
   try {Json raster=j;raster["speed"]=number(j,"speed",1,0,10)*par[1]*10;auto pixels=rasterText(raster,w,h,elapsed);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_BGRA,GL_UNSIGNED_BYTE,pixels.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);cached=key;width=w;height=h;typeFrame=frame;failed=false;}
   catch(const std::exception& e){if(!failed)FFGLLog::LogToHost(e.what());failed=true;{std::lock_guard<std::mutex> l(state->mutex);state->renderError=e.what();}return FF_FAIL;}
  }
  glUseProgram(shader.GetGLID());auto f=[&](const char* name,float value){glUniform1f(shader.FindUniform(name),value);};auto i=[&](const char* name,int value){glUniform1i(shader.FindUniform(name),value);};
  i("glyphs",0);i("video",1);
#ifdef PULSE_EFFECT
  i("effect",1);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,input->inputTextures[0]->Handle);auto max=GetMaxGLTexCoords(*input->inputTextures[0]);glUniform2f(shader.FindUniform("maxUV"),max.s,max.t);
#else
  i("effect",0);
#endif
  i("blackout",black||par[8]>.5f);glUniform2f(shader.FindUniform("resolution"),(float)w,(float)h);
  glUniform2f(shader.FindUniform("position"),number(j,"x",0,-2,2),-number(j,"y",0,-2,2));
  auto colour=[&](const char* key,const char* uniform,float defaultAlpha){float c[4]={1,1,1,defaultAlpha};auto it=j.find(key);if(it!=j.end()&&it->is_array()&&it->size()==4){for(int k=0;k<4;k++)if((*it)[k].is_number()){float v=(*it)[k].get<float>();c[k]=std::isfinite(v)?std::clamp(v,0.f,1.f):0;}}glUniform4f(shader.FindUniform(uniform),c[0],c[1],c[2],c[3]);};
  colour("colour","colour",1);colour("background","background",0);
  f("elapsed",(float)elapsed);f("speed",number(j,"speed",1,0,10)*par[1]*10);f("intensity",number(j,"intensity",1,0,2)*par[2]*2);f("opacity",number(j,"opacity",1,0,1)*par[0]);
  f("scale",number(j,"scale",1,.05f,5));f("rotation",-number(j,"rotation",0,-360,360)*3.14159265f/180);f("outline",number(j,"outline",0,0,20));f("glow",number(j,"glow",0,0,3));
  bool alive=nowSeconds()-last<number(j,"timeout",2,.3f,10);
  float energy=alive?std::clamp(bass*.5f+mid*.3f+treble*.2f,0.f,1.f)*par[3]:0;
  f("audioScale",energy*number(j,"audio_scale",0,0,2));f("audioGlow",energy*number(j,"audio_glow",0,0,3));f("brightness",energy*number(j,"audio_brightness",0,0,2));f("audioShake",energy*number(j,"audio_shake",0,0,2));
  const char* animations[]={"Static","Scroll left","Scroll right","Scroll up","Scroll down","Fade in/out","Zoom pulse","Bounce","Typewriter","Shake"};int animation=0;for(int n=0;n<10;n++)if(word(j,"animation")==animations[n])animation=n;i("animation",animation);
  float flash=0;auto mode=word(j,"strobe_clock","Rate");float rate=number(j,"strobe_rate",4,.1f,30)*par[5]*5;
  if(mode=="BPM")rate=number(j,"bpm",120,20,300)/60*number(j,"subdivision",1,.25f,8);
  if(alive&&!stopped&&flag(j,"strobe")&&par[4]>.5f){if(mode=="Audio")flash=trigger;else flash=std::fmod(elapsed*rate,1.)<number(j,"pulse_width",.2f,.01f,.95f)?1.f:0.f;}
  f("flash",flash*number(j,"strobe_intensity",1,0,1)*par[6]);auto target=word(j,"strobe_target","Text");i("flashMode",target=="Background"?1:target=="Whole output"?2:0);
  quad.Draw();GLenum renderError=glGetError();{std::lock_guard<std::mutex> l(state->mutex);if(renderError!=GL_NO_ERROR){state->renderError="OpenGL render error "+std::to_string(renderError);return FF_FAIL;}state->renderedText=word(j,"text","PulseText");state->renderError.clear();state->frames++;state->lastRender=nowSeconds();}return FF_SUCCESS;
 }
};
#ifdef PULSE_EFFECT
static CFFGLPluginInfo info(PluginFactory<PulseText>,"PuTE","PulseText Effect",2,1,1,0,FF_EFFECT,"Unicode animated text overlay","PulseText 1.0");
#else
static CFFGLPluginInfo info(PluginFactory<PulseText>,"PuTS","PulseText Source",2,1,1,0,FF_SOURCE,"Unicode animated text source","PulseText 1.0");
#endif
