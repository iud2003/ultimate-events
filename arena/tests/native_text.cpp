#include "Text.h"
#include <iostream>
int main(){
 try {
  Json settings={{"text",u8"PulseText 日本語 العربية 🎤\nUnicode"},{"font","Segoe UI"},{"size",48}};
  auto pixels=rasterText(settings,640,360,0);
  size_t visible=0,transparent=0;for(size_t i=3;i<pixels.size();i+=4){if(pixels[i])visible++;else transparent++;}
  if(visible<100||transparent<640*300)return 1;
  settings["text"]="";auto blank=rasterText(settings,640,360,0);
  for(size_t i=3;i<blank.size();i+=4)if(blank[i])return 2;
  std::cout<<"DirectWrite Unicode raster and transparent empty text verified\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 3;}
}
