#pragma once
#include "../../core/controller.hpp"
#include "../../core/wire.hpp"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifndef MG_BUILD_ID
#error Use the N01 build script with a unique MG_BUILD_ID and the NVS erase guard.
#endif
#define MG_STR0(x) #x
#define MG_STR(x) MG_STR0(x)

namespace mg::esp {
inline constexpr char buildId[]=MG_STR(MG_BUILD_ID);
#ifdef MG_LAB
inline constexpr char profile[]="lab";
inline constexpr char journalName[]="lab-v1.journal";
inline constexpr char nvsName[]="mg-lab-v1";
#else
inline constexpr char profile[]="release";
inline constexpr char journalName[]="release-v1.journal";
inline constexpr char nvsName[]="mg-rel-v1";
#endif
inline constexpr size_t provisionMax=8192;
Ms monoNow();
class WallClock : public ClockPort {
public:
    Sample now() override;
    static void synchronized(struct timeval*);
    static void connected(bool);
};
class Output : public OutputPort {
public:
    void initialize();
    void enable(bool);
    void service();
    void touch();
    bool startEdge(bool& busy);
    Drive read() override;
    bool open(Ms,Ms,uint32_t,Drive&) override;
    Drive close(Reason) override;
private:
    portMUX_TYPE lock_=portMUX_INITIALIZER_UNLOCKED;
    Drive drive_;
    bool enabled_=false, startArmed_=false, startSent_=false, edge_=false, busyEdge_=false, timingOkay_=false;
    Ms deadline_=none, beat_=none, startSince_=none, stopRelease_=none;
    uint32_t startStop_=0,edgeStop_=0;
    bool pressBlocked_=false;
    void writePin(bool);
    void closeLocked(Reason,Ms);
    void stopLocked(bool,Ms);
};
class LocalFile : public LogFile {
public:
    bool mount();
    int64_t size() override;
    bool read(uint32_t,uint8_t*,size_t) override;
    bool append(const uint8_t*,size_t) override;
    bool beginReplacement() override;
    bool writeReplacement(const uint8_t*,size_t) override;
    bool publishReplacement() override;
    void discardReplacement() override;
#ifdef MG_LAB
    const char* checkLayout();
#endif
private:
    bool mounted_=false;
    int replacement_=-1;
};
struct Provision {
    Id device;
    Text<32> ssid;
    Text<64> password;
    Text<256> baseUrl;
    Text<256> token;
    std::string ca;
    bool fixtureVerified=false;
};
enum class ProvisionStatus { ok, missing, invalid, storage_error, device_change };
ProvisionStatus loadProvision(Provision&);
#ifdef MG_LAB
ProvisionStatus saveProvision(const char*,size_t);
#endif
BootId newBootId();
}
