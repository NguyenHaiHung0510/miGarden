#pragma once
#ifdef MG_LAB
#include "network.hpp"
#include <esp_ota_ops.h>
namespace mg::esp {
class LabUsb {
public:
    LabUsb(Controller& c,Network& n,LocalFile& f) : controller_(c),network_(n),file_(f) {}
    void begin();
    void visit();
private:
    Controller& controller_;
    Network& network_;
    LocalFile& file_;
    char line_[128]{};
    size_t length_=0;
    bool discard_=false, resync_=false, uncertain_=false, healthy_=false;
    Ms lastInput_=0,lastBeat_=0,booted_=0;
    void heartbeat();
    bool maintenance();
    void command();
    void update(size_t,const uint8_t*,const char*);
    void provision(size_t);
    void fail(const char*,size_t);
    const esp_partition_t* target() const;
};
}
#endif
