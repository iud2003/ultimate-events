#include "Receiver.h"
#include <fstream>
// Standalone harness exercises the actual module receiver without Arena/GL.
int main(int argc,char** argv){
 auto first=std::make_shared<State>();first->id=7;
 auto second=std::make_shared<State>();second->id=8;
 Receiver receiver(12345);receiver.add(first);receiver.add(second);
 if(argc>1&&std::string(argv[1])=="--timeout"){
  first->settings={{"strobe",true},{"timeout",.5}};first->stopped=false;first->heartbeat=10;
  expireStrobe(*first,10.4);if(first->stopped)return 1;
  expireStrobe(*first,10.6);if(!first->stopped||flag(first->settings,"strobe"))return 2;
  first->heartbeat=11;expireStrobe(*first,11);if(!first->stopped)return 3;
  return 0;
 }
 for(int n=0;n<100;n++){
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::lock_guard<std::mutex> a(first->mutex);std::lock_guard<std::mutex> b(second->mutex);
  if(word(first->settings,"text")=="native routing verified"&&second->settings.empty())return 0;
 }
 return 4;
}
