#include "src/platform/esp32/platform.hpp"
#include "src/platform/esp32/network.hpp"
#include "src/platform/esp32/lab_usb.hpp"
#include <memory>

namespace {
using namespace mg;
using namespace mg::esp;
WallClock clockPort;
Output output;
LocalFile file;
Journal journal(file);
Network network;
Controller* controller=nullptr;
#ifdef MG_LAB
LabUsb* lab=nullptr;
#endif

#ifndef MG_COMMISSION
void outputService(void*) {
    auto last=xTaskGetTickCount();
    for (;;) {
        output.service();
        vTaskDelayUntil(&last,pdMS_TO_TICKS(5));
    }
}
#endif
}

size_t getArduinoLoopTaskStackSize() { return 12288; }

void setup() {
    output.initialize();
#ifndef MG_COMMISSION
    file.mount();
#endif
    auto* c=new (std::nothrow) Controller(clockPort,output,journal,newBootId());
    if (!c) return; // hardware was already made inactive
    controller=c;
#ifndef MG_COMMISSION
    c->boot();
    const bool timer=xTaskCreatePinnedToCore(outputService,"mg-output",3072,nullptr,5,nullptr,1)==pdPASS;
    Provision p;
    const auto provision=loadProvision(p);
    if (provision==ProvisionStatus::ok) {
        output.enable(timer && p.fixtureVerified);
        network.begin(std::move(p));
    }
#endif
#ifdef MG_LAB
    lab=new (std::nothrow) LabUsb(*c,network,file);
    if (lab) lab->begin();
#ifdef MG_COMMISSION
    Serial.println("COMMISSIONER_READY MG-COMMISSION-V1 no_control=1 gpio=inactive");
#endif
#else
    Serial.begin(115200);
    enableLoopWDT();
#endif
}

void loop() {
    if (!controller) { delay(20); return; }
#ifdef MG_COMMISSION
    if(lab)lab->visit();
    delay(5);return;
#else
    output.touch();
    bool wasBusy=false;
    const bool start=output.startEdge(wasBusy);
    std::unique_ptr<Network::Reply> in(network.take());
    if (in && in->accepted) controller->tick(start,wasBusy,&in->exchange,in->sent);
    else controller->tick(start,wasBusy);
    if (in) network.after(*in,*controller);
    network.submit(*controller);
#ifdef MG_LAB
    if (lab) lab->visit();
#else
    // Diagnostics are output-only and have no configuration values or identifiers.
    static Ms last=0;
    const auto now=monoNow();
    if (now-last>=30000) {
        last=now;
        const auto s=controller->snapshot();
        Serial.printf("MG build=%s profile=release ready=%u output=%s reason=%s time=%s queue=%u gaps=%llu network=%s wifi_slot=%u wifi_phase=%s\n",
                      buildId,s.ready,s.on?"on":"off",s.blocked==Reason::none?"NONE":label(s.blocked),
                      label(s.time.quality),unsigned(controller->queued()),
                      static_cast<unsigned long long>(controller->evidenceGaps()),network.status(),
                      network.wifiSlot(),network.wifiPhase());
    }
#endif
    delay(5);
#endif
}
