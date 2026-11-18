// Compile only against the retained 57b2156 core, never a new encoder.
#include "fixture.hpp"
#include <fstream>
#include <iostream>
using namespace mg;
using namespace mg::test;
bool put(Fixture& f,const std::string& path) {
    std::ofstream out(path,std::ios::binary);out.write((const char*)f.storage.live.data(),f.storage.live.size());return out.good();
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const std::string dir=argv[1];
    Fixture f;f.apply(f.config());f.tick(true);f.advance(39*second);f.stop("known-39s");
    while(f.c->queued()){f.offer();f.ack();}
    f.tick(true);f.reboot(false);
    if(!put(f,dir+"/proof57b.journal"))return 2;
    while(f.c->queued()){f.offer();f.ack();}
    if(!put(f,dir+"/acked57b.journal"))return 2;
    for(int i=0;i<100;++i)f.tick(true,true);
    while(f.c->queued()){f.offer();f.ack();}
    if(!f.journal->prepare(18)||!f.storage.replacements||!put(f,dir+"/compacted57b.journal"))return 2;
    Fixture unsafe;unsafe.apply(unsafe.config());unsafe.tick(true);unsafe.advance(39*second);unsafe.stop("known-39s");
    unsafe.tick(true);unsafe.actuator.d.usable=false;unsafe.gap(1000);
    if(!put(unsafe,dir+"/unsafe57b.journal"))return 2;
    std::cout<<"PASS four genuine 57b source-generated legacy journals\n";
}
