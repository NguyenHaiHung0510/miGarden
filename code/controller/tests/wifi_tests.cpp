#include "fixture.hpp"
#include "../MiGardenController/src/connectivity/wifi_profile.hpp"
#include "../MiGardenController/src/connectivity/wifi_policy.hpp"
#include <iostream>
#include <stdexcept>
using namespace mg;
using namespace mg::test;
using A=WifiPolicy::Action;
using P=WifiPolicy::Phase;
#define CHECK(x) do{if(!(x))throw std::runtime_error("assertion line "+std::to_string(__LINE__));}while(false)

void preferredHealthyNoFallbackWarning() {
    WifiPolicy w(true);
    CHECK(w.step(0,false)==A::preferred&&w.slot()==1);
    CHECK(w.step(100,true)==A::none&&w.phase()==P::connected);
    for(Ms t=1000;t<24*day;t+=minute)CHECK(w.step(t,true)==A::none&&w.slot()==1);
    CHECK(std::string_view(WifiPolicy::label(w.phase()))=="CONNECTED");
}
void preferredUnavailableThenFallbackSticky() {
    WifiPolicy w(true);
    CHECK(w.step(0,false)==A::preferred);
    CHECK(w.step(14999,false)==A::none);
    CHECK(w.step(15000,false)==A::fallback&&w.slot()==2);
    CHECK(w.step(15100,true)==A::none&&w.phase()==P::connected);
    for(Ms t=16000;t<3*day;t+=minute)CHECK(w.step(t,true)==A::none&&w.slot()==2);
}
void disconnectRestartsPreferredAndBackoff() {
    WifiPolicy w(true);
    w.step(0,false);w.step(15000,false);w.step(16000,true);
    CHECK(w.step(17000,false,true)==A::preferred&&w.slot()==1);
    w.step(18000,true);
    CHECK(w.step(19000,true,true)==A::preferred); // loss event even between status polls
    CHECK(w.step(34000,false)==A::fallback);
    CHECK(w.step(49000,false)==A::disconnect);
    CHECK(w.step(53999,false)==A::none);
    CHECK(w.step(54000,false)==A::preferred);
}
void bothDownFiniteBackoff() {
    WifiPolicy w(true);
    Ms start=0;
    for(Ms pause:{5000,10000,20000,40000,60000,60000,60000}) {
        CHECK(w.step(start,false)==A::preferred);
        CHECK(w.step(start+14999,false)==A::none);
        CHECK(w.step(start+15000,false)==A::fallback);
        CHECK(w.step(start+29999,false)==A::none);
        CHECK(w.step(start+30000,false)==A::disconnect&&w.phase()==P::backoff&&w.slot()==0);
        for(Ms t=start+30001;t<start+30000+pause;t+=100)
            CHECK(w.step(t,false)==A::none);
        start+=30000+pause;
    }
}
void legacyOneNetworkBackoff() {
    WifiPolicy w(false);
    CHECK(w.step(0,false)==A::preferred);
    CHECK(w.step(15000,false)==A::disconnect);
    CHECK(w.step(19999,false)==A::none);
    CHECK(w.step(20000,false)==A::preferred);
    CHECK(w.step(20001,true)==A::none&&w.slot()==1);
}
JsonDocument fields(bool fallback=false) {
    JsonDocument d;
    d["wifi_ssid"]="SYNTHETIC preferred";
    d["wifi_password"]="synthetic pass";
    if(fallback) {
        d["wifi_fallback_ssid"]="SYNTHETIC secondary";
        d["wifi_fallback_password"]="synthetic alternate";
    }
    return d;
}
bool decode(const JsonDocument& doc,WifiProfile& p){return decodeWifiProfile(doc.as<JsonObjectConst>(),p);}
void legacyProfileAndOptionalPair() {
    auto old=fields();auto both=fields(true);WifiProfile p;
    CHECK(decode(old,p)&&!p.hasFallback);
    CHECK(decode(both,p)&&p.hasFallback&&p.fallbackSsid!=p.ssid);
    CHECK(decode(old,p)&&!p.hasFallback&&p.fallbackSsid.empty()&&p.fallbackPassword.empty());
    both["wifi_password"]="";both["wifi_fallback_password"]="";
    CHECK(decode(both,p)); // legacy open-network representation stays supported
}
void pairAndDuplicateFailThenRestore() {
    for(int n=0;n<6;++n) {
        auto bad=fields(true);WifiProfile p;
        if(n==0)bad.remove("wifi_fallback_ssid");
        if(n==1)bad.remove("wifi_fallback_password");
        if(n==2)bad["wifi_fallback_ssid"]=nullptr;
        if(n==3)bad["wifi_fallback_password"]=nullptr;
        if(n==4)bad["wifi_fallback_ssid"]=bad["wifi_ssid"];
        if(n==5)bad["wifi_fallback_ssid"]="";
        CHECK(!decode(bad,p));
        auto good=fields(true);CHECK(decode(good,p)&&p.hasFallback);
    }
}
void sizesAsciiAndPasswordFailThenRestore() {
    for(bool secondary:{false,true}) {
        const char* ssid=secondary?"wifi_fallback_ssid":"wifi_ssid";
        const char* pass=secondary?"wifi_fallback_password":"wifi_password";
        for(const std::string& value:{std::string(33,'x'),std::string("bad\nname"),std::string("bad\x7f"),std::string("bad\xc3\xa9")}) {
            auto bad=fields(true);bad[ssid]=value;WifiProfile p;
            CHECK(!decode(bad,p));
            bad[ssid]=std::string(32,'x');CHECK(decode(bad,p));
        }
        for(const std::string& value:{std::string(7,'x'),std::string(65,'x'),std::string(64,'z'),
                                     std::string("bad\npass"),std::string("bad\x7fpass"),std::string("bad\xc3\xa9pass")}) {
            auto bad=fields(true);bad[pass]=value;WifiProfile p;CHECK(!decode(bad,p));
            bad[pass]=std::string(64,'a');CHECK(decode(bad,p));
        }
        auto good=fields(true);good[pass]=std::string(63,'x');WifiProfile p;CHECK(decode(good,p));
    }
}
void offlineConnectAttemptsDoNotBlockWatering() {
    Fixture f;auto c=f.config();c.schedule.type=Recurrence::once;c.schedule.enabled=true;
    c.schedule.duration=15;c.schedule.at=baseTime+minute;c.schedule.localMinute=0;
    CHECK(valid(c));
    f.apply(c);
    WifiPolicy wifi(true);
    unsigned attempts=0;
    for(Ms t=0;t<2*minute;t+=100) {
        const auto action=wifi.step(f.clock.s.mono,false);
        if(action==A::preferred||action==A::fallback)++attempts;
        f.clock.elapse(100);f.tick(t==0); // manual START while preferred is trying
        if(t==1000) {
            f.actuator.pressStop();f.tick();f.actuator.releaseStop();
            CHECK(!f.c->snapshot().on);
        }
    }
    CHECK(attempts>=4&&f.actuator.opens==2&&!f.c->snapshot().on);
    CHECK(f.c->ledger().history.size()==2);
    CHECK(f.c->ledger().history.back().end-f.c->ledger().history.back().start==15*second);
}
int main() {
    unsigned passed=0,failed=0;
    auto test=[&](const char* name,auto fn) {
        try{fn();++passed;std::cout<<"PASS "<<name<<"\n";}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<"\n";}
    };
    test("preferredHealthyNoFallbackWarning",preferredHealthyNoFallbackWarning);
    test("preferredUnavailableThenFallbackSticky",preferredUnavailableThenFallbackSticky);
    test("disconnectRestartsPreferredAndBackoff",disconnectRestartsPreferredAndBackoff);
    test("bothDownFiniteBackoff",bothDownFiniteBackoff);
    test("legacyOneNetworkBackoff",legacyOneNetworkBackoff);
    test("legacyProfileAndOptionalPair",legacyProfileAndOptionalPair);
    test("pairAndDuplicateFailThenRestore",pairAndDuplicateFailThenRestore);
    test("sizesAsciiAndPasswordFailThenRestore",sizesAsciiAndPasswordFailThenRestore);
    test("offlineConnectAttemptsDoNotBlockWatering",offlineConnectAttemptsDoNotBlockWatering);
    std::cout<<"TOTAL "<<passed<<" PASS, "<<failed<<" FAIL\n";
    return failed!=0;
}
