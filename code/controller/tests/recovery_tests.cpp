#include "fixture.hpp"
#include "wire.hpp"
#include "sync_policy.hpp"
#include <ArduinoJson.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace mg;
using namespace mg::test;
#define CHECK(x) do{if(!(x))throw std::runtime_error("assertion line "+std::to_string(__LINE__));}while(false)
unsigned recovered(const std::vector<Event>& events) {
    unsigned n=0;for(const auto& e:events)if(e.kind==Kind::recovered)++n;return n;
}
void rebootThenNewStopRecovery() {
    Fixture f;f.apply(f.config());f.tick(true);const auto original=f.c->ledger().session;
    f.reboot(false);
    CHECK(!f.c->snapshot().on&&!f.c->snapshot().ready&&f.c->ledger().unknown);
    f.stop("explicit-after-boot");
    CHECK(!f.c->ledger().unknown&&f.c->snapshot().ready&&!f.c->snapshot().on);
    const auto events=f.offer();CHECK(recovered(events)==1);
    const auto* old=find(events,Kind::session_ended,Reason::reboot_interrupted);
    CHECK(old&&old->outcome==Outcome::unknown&&old->outputMs==none);
    const auto* proof=find(events,Kind::recovered);
    CHECK(proof&&proof->session==original.id&&proof->request==original.request&&proof->config==original.config&&
          proof->source==original.source&&proof->boot==f.c->bootId()&&proof->outputMs==none);
    CHECK(std::string_view(label(proof->reason))=="REBOOT_INTERRUPTED_RECOVERED");
}
#ifndef MG_RECOVERY_BASELINE
void idleBootNoAutomaticOpen() {
    Fixture f;f.apply(f.config());f.reboot(false);
    CHECK(f.c->snapshot().ready&&!f.c->snapshot().on&&!f.actuator.opens&&!f.c->ledger().unknown);
}
void localRemoteArmedAndOldGrant() {
    for(bool remote:{false,true})for(bool armed:{false,true}) {
        Fixture f;f.apply(f.config());
        auto intent=f.intent("original",900);
        std::vector<uint8_t> reservation;
        f.actuator.beforeOpen=[&]{reservation=f.storage.live;};
        if(remote){f.candidate(intent);f.offer();f.grant(intent);}else f.tick(true);
        if(armed)f.storage.live=reservation;
        const auto opens=f.actuator.opens;
        f.reboot(false);CHECK(!f.c->snapshot().on&&f.c->ledger().unknown);
        const auto expected=armed?RecoveryCause::reboot_armed:RecoveryCause::reboot_running;
        CHECK(f.c->ledger().recoveryCause==expected);
        const auto reports=f.offer();
        const auto* terminal=find(reports,Kind::session_ended,Reason::reboot_interrupted);
        CHECK(terminal&&terminal->outcome==Outcome::unknown&&terminal->outputMs==none&&
              (terminal->started==none)==armed);
        if(armed)CHECK(!find(reports,Kind::session_started));
        if(remote){f.actuator.pressStop();f.tick();f.actuator.releaseStop();f.tick();}
        else f.stop("new-stop");
        CHECK(!f.c->ledger().unknown&&f.c->ledger().recoveryDebt==900*second);
        f.grant(intent);CHECK(!f.c->snapshot().on&&f.actuator.opens==opens);
        f.tick(true);CHECK(f.actuator.opens==opens+1); // new ordinary START retains remaining budget
    }
}
void preserveBudgetHistoryAndNoDoubleCharge() {
    Fixture f;f.apply(f.config());f.tick(true);f.advance(39*second);f.stop("measured");
    const auto retained=f.c->ledger().history;
    f.tick(true);const auto original=f.c->ledger().session;
    f.reboot(false);const auto floor=f.c->ledger().accountingFloor;
    f.stop("settle");
    CHECK(f.c->ledger().accountingFloor==floor&&f.c->ledger().history.size()==retained.size());
    CHECK(f.c->ledger().history[0].start==retained[0].start&&f.c->ledger().history[0].end==retained[0].end);
    CHECK(f.c->ledger().recoveryDebt==Ms(original.duration)*second&&recovered(f.offer())==1);
    const auto debt=f.c->ledger().recoveryDebt;
    f.stop("settle");f.stop("another");CHECK(f.c->ledger().recoveryDebt==debt&&recovered(f.offer())==1);
    f.offer();f.ack();f.reboot(false);f.stop("new-boot-stop");
    CHECK(f.c->ledger().recoveryDebt==debt&&!f.c->ledger().unknown&&recovered(f.offer())==0);
    f.tick(true);CHECK(f.c->snapshot().on);
}
void debtExpiryOnlyPoweredUptime() {
    Fixture f;f.apply(f.config());f.tick(true);f.reboot(false);f.stop();
    const auto debt=f.c->ledger().recoveryDebt;
    f.clock.s.wall+=10*day;f.tick();f.reboot(false);
    CHECK(recoveryCharge(f.c->ledger(),f.c->ledger().accountingFloor)==debt);
    f.advance(windowSeconds*second-1);
    CHECK(recoveryCharge(f.c->ledger(),f.c->ledger().accountingFloor+windowSeconds*second-1)==debt);
    f.advance(1);f.tick(true);CHECK(f.c->snapshot().on&&f.c->ledger().recoveryDebt==0);
}
void fullBudgetStillRefusesAndOtherUsageNotAged() {
    Fixture f;auto cfg=f.config("tight",{900,1800,windowSeconds,1800});f.apply(cfg);
    f.tick(true);f.advance(900*second);
    CHECK(!f.c->snapshot().on);
    f.tick(true);f.reboot(false);const auto floor=f.c->ledger().accountingFloor;
    f.stop();CHECK(!f.c->ledger().unknown&&f.c->ledger().accountingFloor==floor);
    const auto opens=f.actuator.opens;f.tick(true);
    CHECK(!f.c->snapshot().on&&f.actuator.opens==opens&&find(f.offer(),Kind::request_rejected,Reason::rolling_limit));
}
void failedCommitUnsafeAndStorageRemainBlocked() {
    Fixture f;f.apply(f.config());f.tick(true);f.reboot(false);
    f.storage.failAt=f.storage.appendNumber+1;f.stop();
    CHECK(f.c->ledger().unknown&&!f.c->snapshot().on&&recovered(f.offer())==0);
    Fixture u;u.apply(u.config());u.tick(true);u.actuator.d.usable=false;u.gap(1000);
    u.actuator.d.usable=true;u.stop();u.reboot(false);u.stop("again");
    CHECK(u.c->ledger().unknown&&u.c->ledger().recoveryCause==RecoveryCause::other);
    Fixture s;s.apply(s.config());s.tick(true);s.reboot(false);s.storage.live.back()^=1;s.stop();
    CHECK(s.c->ledger().unknown&&!s.c->snapshot().on);
}
void tornCommitAndDurableLostReceipt() {
    Fixture torn;torn.apply(torn.config());torn.tick(true);torn.reboot(false);
    torn.storage.partialAt=torn.storage.appendNumber+1;torn.stop();
    CHECK(torn.c->ledger().unknown&&!torn.c->snapshot().on);
    torn.reboot(false);torn.stop("again");
    CHECK(!torn.c->snapshot().ready&&torn.c->snapshot().blocked==Reason::storage_fault&&recovered(torn.offer())==0);
    Fixture durable;durable.apply(durable.config());durable.tick(true);durable.reboot(false);
    durable.storage.afterAppend=[] {throw std::runtime_error("synthetic-power-cut");};
    bool cut=false;
    try{durable.stop();}catch(const std::runtime_error&){cut=true;}
    durable.storage.afterAppend={};CHECK(cut);durable.reboot(false);
    CHECK(!durable.c->ledger().unknown&&durable.c->ledger().recoveryDebt==900*second&&recovered(durable.offer())==1);
    durable.stop("retry");CHECK(durable.c->ledger().recoveryDebt==900*second&&recovered(durable.offer())==1);
}
void recoveredWireAndFullLedgerMetadata() {
    Fixture f;f.apply(f.config());f.tick(true);const auto original=f.c->ledger().session;
    f.reboot(false);f.stop();
    const auto reports=f.offer();std::string wire;
    CHECK(encodeExchange(id("synthetic-device"),f.c->bootId(),f.c->snapshot(),reports,wire));
    JsonDocument doc;CHECK(!deserializeJson(doc,wire));
    bool seen=false;
    for(JsonObjectConst e:doc["reports"].as<JsonArrayConst>())if(e["kind"]=="recovered") {
        CHECK(e["session_id"]==original.id.view()&&e["request_id"]==original.request.view()&&
              e["config_ref"]==original.config.view()&&e["trigger_source"]==label(original.source));
        auto p=e["payload"].as<JsonObjectConst>();
        CHECK(p.size()==1&&p["reason"]=="REBOOT_INTERRUPTED_RECOVERED");seen=true;
    }
    CHECK(seen);
    Memory store;Journal j(store);Ledger l;CHECK(j.restore(l));
    l.unknown=true;l.recoveryCause=RecoveryCause::other;l.recoveryDebt=900*second;l.recoveryDebtAt=100000;
    l.applied.ref=id(std::string(128,'c'));l.accountingFloor=100000;
    for(size_t n=0;n<intervalMax;++n)l.history.push_back({Ms(n*2),Ms(n*2+1)});
    for(size_t n=0;n<configHistoryMax-1;++n)l.configRefs.push_back(n);
    l.configRefs.push_back(idFingerprint(l.applied.ref.view()));
    CHECK(j.commit(&l));Journal read(store);Ledger restored;CHECK(read.restore(restored));
    CHECK(restored.history.size()==intervalMax&&restored.recoveryDebt==900*second&&restored.recoveryCause==RecoveryCause::other);
}
void heldAtBootAndStaleStopCannotSettle() {
    Fixture f;f.apply(f.config());f.tick(true);
    const auto old=f.c->bootId();
    f.actuator.pressStop();f.clock.s.mono=0;f.boot(); // preserve held STOP at the boot sample
    CHECK(f.c->ledger().unknown&&recovered(f.offer())==0);
    Exchange stale;stale.stops={{id("old-stop"),old,f.clock.s.wall,true}};f.exchange(stale);
    CHECK(f.c->ledger().unknown);
    f.actuator.releaseStop();f.tick();f.actuator.pressStop();f.tick();
    CHECK(!f.c->ledger().unknown&&recovered(f.offer())==1);
}
void legacyFiles(const char* directory) {
    for(const auto* name:{"proof57b","acked57b","compacted57b","unsafe57b"}) {
        Fixture f;
        std::ifstream input(std::string(directory)+"/"+name+".journal",std::ios::binary);
        CHECK(input.good());f.storage.live.assign(std::istreambuf_iterator<char>(input),{});
        const auto before=f.storage.live;
        f.reboot(false);
        const bool proven=std::string_view(name)=="proof57b"||std::string_view(name)=="acked57b";
        CHECK((f.c->ledger().recoveryCause==RecoveryCause::reboot_running)==proven);
        const auto history=f.c->ledger().history;
        f.stop("explicit-legacy");
        CHECK(f.c->ledger().unknown!=proven);
        CHECK(f.c->ledger().history.size()==history.size()&&f.c->ledger().history[0].end-history[0].start==39*second);
        if(proven)CHECK(f.c->ledger().recoveryDebt==900*second&&recovered(f.offer())==1);
        // Source-generated fixtures only. No production copy is read by this test.
        CHECK(!before.empty());
    }
}
void hintPreemptionAndErrorBounds() {
    Exchange x;x.retryAfter=2;x.serverTime=baseTime;CHECK(foregroundHint(x,true,true));
    CHECK(!foregroundHint(x,false,true)&&!foregroundHint(x,true,false));
    for(int n=0;n<6;++n) {
        auto busy=x;
        if(n==0)busy.hasConfig=true;
        if(n==1)busy.invalidConfig=true;
        if(n==2)busy.candidates.push_back({});
        if(n==3)busy.grants.push_back({});
        if(n==4)busy.stops.push_back({});
        if(n==5)busy.acked.push_back({});
        CHECK(!foregroundHint(busy,true,true));
    }
    x.serverTime=none;CHECK(!foregroundHint(x,true,true));
    SyncPolicy p;CHECK(p.due(0,true,1));p.sent(1);
    p.ok(0,false,false,2,-1,true);
    CHECK(!p.due(1999,true,1)&&p.due(2000,true,1)&&p.due(1,true,2));
    p.sent(2);p.ok(1,false,true,2,-1,true);CHECK(p.due(1,true,2));
    p.ok(10,false,false,-1);CHECK(p.next()==15010&&!p.due(2011,true,2));
    p.ok(100,true,false,2,-1,true);CHECK(p.next()==5100); // no high poll in active base
    p.ok(200,false,true,2,20,true);CHECK(p.next()==20200&&!p.due(201,true,3));
    p.transient(300,0,2);CHECK(p.next()==5300&&!p.due(301,true,4));
    CHECK(!p.due(302,false,4)&&!p.due(303,true,4)); // reconnect cannot erase error backoff
    p.transient(5300,1000,70);CHECK(p.next()==75300&&!p.due(5400,true,5));
}
#endif
int main(int argc,char** argv) {
    unsigned passed=0,failed=0;
    auto test=[&](const char* name,auto fn) {
        try{fn();++passed;std::cout<<"PASS "<<name<<"\n";}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<" "<<e.what()<<"\n";}
    };
    test("rebootThenNewStopRecovery",rebootThenNewStopRecovery);
#ifndef MG_RECOVERY_BASELINE
    test("idleBootNoAutomaticOpen",idleBootNoAutomaticOpen);
    test("localRemoteArmedAndOldGrant",localRemoteArmedAndOldGrant);
    test("preserveBudgetHistoryAndNoDoubleCharge",preserveBudgetHistoryAndNoDoubleCharge);
    test("debtExpiryOnlyPoweredUptime",debtExpiryOnlyPoweredUptime);
    test("fullBudgetStillRefusesAndOtherUsageNotAged",fullBudgetStillRefusesAndOtherUsageNotAged);
    test("failedCommitUnsafeAndStorageRemainBlocked",failedCommitUnsafeAndStorageRemainBlocked);
    test("tornCommitAndDurableLostReceipt",tornCommitAndDurableLostReceipt);
    test("recoveredWireAndFullLedgerMetadata",recoveredWireAndFullLedgerMetadata);
    test("heldAtBootAndStaleStopCannotSettle",heldAtBootAndStaleStopCannotSettle);
    test("legacyFiles",[&]{CHECK(argc==2);legacyFiles(argv[1]);});
    test("hintPreemptionAndErrorBounds",hintPreemptionAndErrorBounds);
#else
    (void)argc;(void)argv;
#endif
    std::cout<<"TOTAL "<<passed<<" PASS, "<<failed<<" FAIL\n";return failed!=0;
}
