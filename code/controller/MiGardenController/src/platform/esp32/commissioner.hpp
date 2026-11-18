#pragma once
#ifdef MG_COMMISSION
#ifndef MG_LAB
#error Commissioner is a temporary GPIO-inactive USB laboratory image.
#endif
#include "platform.hpp"
#include "../../commissioning/transaction.hpp"
#include <esp_partition.h>
namespace mg::esp {
class CommissionPort : public commission::Port {
public:
    bool observe(commission::Observation&) override;
    void target(bool lab)override{lab_=lab;}
    bool formatBlank() override;
    bool createJournal() override;
    bool createNamespace(std::string_view) override;
    bool verifyCreated(commission::Observation&) override;
    const esp_partition_t* region(const char*) const;
    bool rawHash(const esp_partition_t*,commission::Digest&,bool* allFF=nullptr);
private:
    bool mounted_=false,readOnly_=false;
    bool lab_=false;
    commission::Digest lastData_;
    bool mount(bool);
    bool nvsInventory(commission::Digest&);
    bool filesInventory(commission::Digest&);
};
class Commissioner {
public:
    Commissioner():transaction_(port_){}
    bool command(char*);
    bool mayBeginUpdate(size_t,const char*)const;
    bool approvedTarget(size_t,const char*)const;
    static Commissioner& instance();
private:
    CommissionPort port_;
    commission::Transaction transaction_;
    bool commandFailed_=false;
    bool verified_=false;
    commission::Plan completed_;
    bool snapshotOkay_=false;
    commission::Observation snapshot_;
    void status();
    void process(const char*,size_t);
    void backup(const char*);
    void frame(size_t,const char*);
};
}
#endif
