#include "fixture.hpp"
#include "sync_policy.hpp"
#include "wire.hpp"
#include <ArduinoJson.h>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace mg;
using namespace mg::test;

#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(#x)+" (line "+std::to_string(__LINE__)+")"); } while (false)

void configAndBoot() {
    Fixture f;
    f.tick(true);
    CHECK(f.c->snapshot().blocked==Reason::config_missing && !f.actuator.opens);
    f.clock.s.quality=Quality::unknown;
    f.apply(f.config());
    CHECK(f.c->snapshot().applied==id("cfg-1"));
    CHECK(f.c->snapshot().blocked==Reason::time_unknown && !f.actuator.opens);
    f.reboot();
    CHECK(f.c->snapshot().applied==id("cfg-1") && !f.actuator.opens);
    f.clock.s.quality=Quality::holdover;
    f.tick();
    CHECK(f.c->snapshot().ready);
    CHECK(find(f.offer(),Kind::config_applied));
}
void defaultsAndDeadline() {
    Fixture f;
    f.apply(f.config());
    CHECK(f.c->ledger().applied.policy.manual==900);
    CHECK(f.c->ledger().applied.policy.session==1800);
    CHECK(f.c->ledger().applied.policy.cumulative==7200);
    f.tick(true);
    CHECK(f.c->snapshot().active.duration==900);
    f.clock.s.quality=Quality::holdover;
    f.gap(901*second);
    const auto reports=f.offer();
    const auto* end=find(reports,Kind::session_ended);
    CHECK(f.actuator.opens==1 && !f.c->snapshot().on && end);
    CHECK(end->outcome==Outcome::completed && end->outputMs==900*second);
}
void configInvalidConflictAndStale() {
    Fixture f;
    auto bad=f.config("bad"); bad.policy.window=3600;
    f.apply(bad);
    CHECK(f.c->snapshot().applied.empty());
    CHECK(find(f.offer(),Kind::config_rejected,Reason::invalid_config));
    auto a=f.config("a"); f.apply(a);
    auto conflict=a; conflict.policy.manual=180;
    f.apply(conflict);
    auto b=f.config("b"); f.apply(b);
    f.apply(a);
    CHECK(f.c->snapshot().applied==b.ref);
    const auto events=f.offer();
    CHECK(find(events,Kind::config_rejected,Reason::config_conflict));
    CHECK(find(events,Kind::config_rejected,Reason::stale_config));
    CHECK(f.actuator.opens==0);
}
void applyCutover() {
    Fixture f;
    auto a=f.config("a",{15,45,windowSeconds,7200});
    a.schedule={true,45,0,none,Recurrence::interval,0,0,baseTime+minute,1};
    f.apply(a);
    f.advance(minute);
    CHECK(f.actuator.opens==1 && f.c->snapshot().active.duration==45);
    auto b=a; b.ref=id("b"); b.schedule.enabled=false; b.policy={15,15,windowSeconds,15}; b.schedule.duration=15;
    f.apply(b);
    CHECK(f.c->snapshot().applied==a.ref && f.c->snapshot().active.duration==45);
    f.advance(17*second); f.stop();
    f.apply(b);
    CHECK(f.c->snapshot().applied==b.ref && !f.c->snapshot().on);
    CHECK(rollingUsage(f.c->ledger().history,f.clock.s.wall)==17*second);
    f.tick(true);
    auto e=f.offer();
    CHECK(f.actuator.opens==1 && find(e,Kind::request_rejected,Reason::rolling_limit));
    const auto* end=find(e,Kind::session_ended);
    CHECK(end && end->config==a.ref && end->outputMs==17000);
    b.ref=id("c"); b.policy.cumulative=45;
    f.apply(b); f.tick(true);
    CHECK(f.actuator.opens==2);
}
void exactBudgetOracle() {
    const auto W=windowSeconds*second;
    CHECK(rollingUsage({{0,20*minute},{50*minute,70*minute},{90*minute,105*minute}},190*minute)==45*minute);
    // At start the old 120 min are all still in the window; during this new
    // run an equal slice exits as the new interval enters.
    CHECK(rollingPeak({{0,120*minute}},{W,W+30*minute})==120*minute);
    CHECK(rollingPeak({{0,120*minute}},{W-minute,W+29*minute})==121*minute);
    std::mt19937 g(919);
    for (int i=0;i<200;++i) {
        std::vector<Range> previous;
        Ms stamp=0;
        for (int j=0;j<6;++j) {
            stamp+=g()%1000*second;
            const auto e=stamp+Ms(g()%3000+1)*second;
            previous.push_back({stamp,e});
            stamp=e;
        }
        stamp+=g()%1000*second;
        const auto e=stamp+Ms(g()%200+1)*second;
        auto all=previous; all.push_back({stamp,e});
        Ms brute=0;
        for (auto t=stamp; t<=e+second; t+=second) brute=std::max(brute,rollingUsage(all,t));
        CHECK(rollingPeak(previous,{stamp,e})==brute);
    }
}
void allSourcesOneBudget() {
    Fixture f;
    auto v=f.config("three",{15,15,windowSeconds,45});
    v.schedule={true,15,0,none,Recurrence::once,0,0,baseTime+minute,0};
    f.apply(v);
    f.tick(true); f.advance(15*second);
    auto candidate=f.intent(); f.candidate(candidate); f.offer(); f.grant(candidate); f.advance(15*second);
    f.advance(30*second);
    CHECK(f.actuator.opens==3 && f.c->snapshot().active.source==Source::schedule);
    f.advance(15*second); f.tick(true);
    const auto events=f.offer();
    CHECK(f.actuator.opens==3 && find(events,Kind::request_rejected,Reason::rolling_limit));
    CHECK(rollingUsage(f.c->ledger().history,f.clock.s.wall)==45*second);
    f.reboot(false); f.tick(true);
    CHECK(f.actuator.opens==3);
}
void sourceBusyIsNotFifo() {
    Fixture f;
    f.apply(f.config("cfg",{15,45,windowSeconds,7200}));
    f.tick(true);
    const auto i=f.intent();
    f.tick(true);
    f.candidate(i); f.offer();
    f.grant(i);
    f.advance(15*second);
    f.candidate(i); f.grant(i);
    f.tick(true,true); // edge latched during the previous session
    CHECK(f.actuator.opens==1);
    const auto events=f.offer();
    CHECK(find(events,Kind::request_rejected,Reason::busy));
    CHECK(!f.c->snapshot().on);
}
void stopWithoutReadinessAndDedup() {
    Fixture f;
    f.clock.s.quality=Quality::unknown;
    f.stop(); const auto epoch=f.c->snapshot().epoch;
    f.stop();
    CHECK(epoch==1 && f.c->snapshot().epoch==epoch);
    const auto e=f.offer();
    const auto* stop=find(e,Kind::stop_processed);
    CHECK(e.size()==1 && stop && stop->outcome==Outcome::already_off && stop->config.empty() && stop->occurred==none);
    f.actuator.pressStop(); f.tick(true);
    CHECK(f.c->snapshot().epoch==epoch+1 && !f.actuator.opens);
}
void stopBeatsGrantAndLocal() {
    Fixture f;
    f.apply(f.config());
    const auto old=f.intent();
    f.candidate(old); f.offer();
    f.actuator.pressStop();
    Exchange x; x.grants={{old,id("grant-1")}}; x.candidates={old};
    f.c->tick(true,false,&x,f.sent);
    f.actuator.releaseStop(); f.tick();
    f.exchange(x);
    CHECK(!f.actuator.opens && !f.c->ledger().pending.receivedSequence);
    const auto current=f.intent("new");
    f.candidate(current); f.offer();
    Exchange simultaneous; simultaneous.grants={{current,id("grant-2")}};
    simultaneous.stops={{id("stop-now"),f.c->bootId(),f.clock.s.wall,true}};
    f.exchange(simultaneous);
    CHECK(!f.actuator.opens && f.c->snapshot().epoch==2);
    f.stop("stop-now"); CHECK(f.c->snapshot().epoch==2);
    f.tick(true); CHECK(f.actuator.opens==1); // fresh local action only
}
void stopBetweenReservationAndOutput() {
    Fixture f;
    f.apply(f.config());
    f.storage.afterAppend=[&] {
        f.actuator.pressStop();
        f.actuator.releaseStop();
    };
    f.tick(true);
    f.storage.afterAppend={};
    f.tick();
    CHECK(f.actuator.opens==0 && f.c->snapshot().epoch==1);
    CHECK(f.c->ledger().session.phase==Session::Phase::idle);
}
void remoteHandoffAndStableReports() {
    Fixture f;
    f.apply(f.config());
    auto i=f.intent();
    f.grant(i); CHECK(!f.actuator.opens);
    f.candidate(i); f.candidate(i);
    f.grant(i); CHECK(!f.actuator.opens);
    auto e=f.offer();
    const auto* received=find(e,Kind::request_received);
    CHECK(received && f.c->snapshot().ready && !find(e,Kind::session_started));
    const auto count=f.c->queued();
    f.candidate(i);
    CHECK(f.c->queued()==count);
    f.grant(i); f.grant(i);
    CHECK(f.actuator.opens==1 && f.c->snapshot().on);
    f.advance(15*second);
    f.candidate(i); f.grant(i);
    CHECK(f.actuator.opens==1 && !f.c->snapshot().on);
    e=f.offer();
    CHECK(find(e,Kind::session_ended));
    auto again=f.offer();
    std::string a,b;
    const auto snapshot=f.c->snapshot();
    CHECK(encodeExchange(id("device"),f.c->bootId(),snapshot,e,a));
    CHECK(encodeExchange(id("device"),f.c->bootId(),snapshot,again,b) && a==b);
    f.ack();
    CHECK(f.c->queued()==0 && rollingUsage(f.c->ledger().history,f.clock.s.wall)==15*second);
    f.reboot(false);
    CHECK(f.c->queued()==0 && rollingUsage(f.c->ledger().history,f.clock.s.wall)==15*second);
}
void expiryAndMismatchedGrant() {
    Fixture f;
    f.apply(f.config("ttl",{15,90,windowSeconds,7200}));
    const auto expired=f.intent();
    f.candidate(expired); f.offer(); f.gap(45*second); f.grant(expired);
    CHECK(f.actuator.opens==0);
    const auto altered=f.intent("altered");
    f.candidate(altered); f.offer(); auto bad=altered; bad.duration=30; f.grant(bad); f.grant(altered);
    CHECK(f.actuator.opens==0);
    const auto delayed=f.intent("late");
    f.candidate(delayed); f.offer();
    f.gap(44*second);
    f.storage.afterAppend=[&] { f.clock.elapse(1100); };
    f.grant(delayed);
    f.storage.afterAppend={};
    CHECK(f.actuator.opens==0);
    CHECK(find(f.offer(),Kind::request_rejected,Reason::expired));
    const auto actual=f.intent("new",60);
    f.candidate(actual); f.offer(); f.grant(actual);
    f.gap(46*second);
    CHECK(f.actuator.opens==1 && f.c->snapshot().on);
    f.gap(14*second); CHECK(!f.c->snapshot().on);
}
void finalOutputTimeFence() {
    Fixture f;
    f.apply(f.config());
    const auto i=f.intent();
    f.candidate(i); f.offer(); f.gap(44*second);
    f.actuator.beforeOpen=[&] { f.clock.elapse(1000); };
    f.grant(i);
    CHECK(f.actuator.opens==0 && f.c->ledger().session.phase==Session::Phase::idle);
}
void wallRollbackCannotRenewHandoff() {
    Fixture f;
    f.apply(f.config());
    const auto i=f.intent();
    f.candidate(i); f.offer(); f.gap(44*second);
    f.clock.s.wall-=20*second;
    f.gap(2*second);
    f.grant(i);
    CHECK(f.actuator.opens==0 && !f.c->ledger().pending.receivedSequence);
    CHECK(find(f.offer(),Kind::request_rejected,Reason::expired));
}
void remoteContextAndReboot() {
    Fixture f;
    f.apply(f.config("old"));
    const auto old=f.intent();
    f.candidate(old); f.offer();
    f.apply(f.config("new")); f.grant(old);
    CHECK(!f.actuator.opens);
    const auto current=f.intent("curr"); f.candidate(current); f.offer();
    f.reboot(false); f.grant(current); f.candidate(current);
    CHECK(!f.actuator.opens && !f.c->ledger().pending.receivedSequence);
    CHECK(find(f.offer(),Kind::request_rejected,Reason::stale_boot));
    Exchange stale; stale.stops={{id("stale-stop"),current.boot,f.clock.s.wall,true}};
    f.tick(true); f.exchange(stale);
    CHECK(f.c->snapshot().on && f.c->snapshot().epoch==0);
}
void unknownAcrossSourcesAndAck() {
    Fixture f;
    auto cfg=f.config("a",{15,45,windowSeconds,150});
    cfg.schedule={true,15,0,none,Recurrence::once,0,0,baseTime+minute,0};
    f.apply(cfg); f.tick(true);
    f.reboot();
    CHECK(f.actuator.opens==1 && !f.c->snapshot().on && f.c->snapshot().blocked==Reason::outcome_unknown);
    CHECK(f.c->ledger().session.phase==Session::Phase::unresolved);
    auto e=f.offer();
    const auto* terminal=find(e,Kind::session_ended,Reason::reboot_interrupted);
    CHECK(terminal && terminal->outcome==Outcome::unknown && terminal->outputMs==none);
    f.ack();
    f.clock.s.quality=Quality::synced; f.tick(true);
    f.advance(minute);
    auto remote=f.intent(); f.candidate(remote); f.offer(); f.grant(remote);
    cfg.ref=id("b"); f.apply(cfg);
    f.gap(4*60*minute); f.tick(true);
    CHECK(f.actuator.opens==1 && f.c->ledger().unknown);
    f.reboot(false); f.tick(true);
    CHECK(f.actuator.opens==1 && f.c->snapshot().blocked==Reason::outcome_unknown);
    f.stop();
    CHECK(!f.c->ledger().unknown&&f.c->ledger().recoveryDebt==15*second&&!f.c->snapshot().on);
}
void persistBeforeOutput() {
    Fixture f;
    f.apply(f.config());
    f.storage.failAt=f.storage.appendNumber+1;
    f.tick(true);
    CHECK(!f.actuator.opens && f.c->snapshot().blocked==Reason::storage_fault);
    Fixture g;
    g.apply(g.config());
    g.actuator.beforeOpen=[&] {
        Journal restore(g.storage); Ledger state;
        CHECK(restore.restore(state));
        CHECK(state.session.phase==Session::Phase::armed && state.session.duration==900 && g.actuator.opens==0);
    };
    g.tick(true); CHECK(g.actuator.opens==1);
}
void failureAfterOutputAndTerminal() {
    Fixture f;
    f.apply(f.config());
    f.storage.failAt=f.storage.appendNumber+2; // reserve durable, event append fails after possible OPEN
    f.tick(true);
    CHECK(f.actuator.opens==1 && !f.c->snapshot().on && f.c->snapshot().blocked==Reason::outcome_unknown);
    f.reboot(false);
    auto events=f.offer();
    const auto* terminal=find(events,Kind::session_ended,Reason::reboot_interrupted);
    CHECK(terminal&&terminal->outcome==Outcome::unknown&&terminal->started==none&&terminal->outputMs==none&&
          !find(events,Kind::session_started));
    CHECK(f.c->ledger().unknown);
    Fixture g;
    g.apply(g.config());
    g.tick(true); g.advance(17*second);
    g.storage.failAt=g.storage.appendNumber+1;
    g.stop();
    CHECK(!g.c->snapshot().on && g.c->snapshot().blocked==Reason::outcome_unknown);
    g.reboot(false);
    CHECK(g.c->ledger().unknown);
}
void damageDuringSession() {
    Fixture f;
    f.apply(f.config());
    f.tick(true);
    f.storage.live.back()^=0x01;
    f.gap(1100);
    CHECK(!f.c->snapshot().on && f.c->snapshot().blocked==Reason::outcome_unknown);
    f.reboot(false);
    CHECK(f.c->snapshot().blocked==Reason::storage_fault);
    f.tick(true); CHECK(f.actuator.opens==1);
}
void corruptionBeforeAdmissionAndMissingStore() {
    Fixture f;
    f.apply(f.config());
    f.storage.live.back()^=0x01;
    f.tick(true); CHECK(f.c->snapshot().blocked==Reason::storage_fault && !f.actuator.opens);
    f.storage.live.clear();
    f.reboot(false); f.tick(true);
    CHECK(!f.actuator.opens && f.storage.live.empty());
    f.actuator.pressStop(); f.tick();
    CHECK(f.c->snapshot().epoch==1);
}
void uncertainClosure() {
    Fixture f;
    f.apply(f.config());
    f.tick(true);
    f.actuator.d.usable=false;
    f.gap(second);
    CHECK(f.c->ledger().unknown && !f.c->snapshot().on);
    const auto reports=f.offer();
    const auto* e=find(reports,Kind::session_ended,Reason::unsafe_output);
    CHECK(e && e->outcome==Outcome::unknown && e->outputMs==none);
}
void schedulePatternsAndCutover() {
    CHECK(due({true,15,0,none,Recurrence::daily,420,0,none,0},baseTime));
    CHECK(due({true,15,0,none,Recurrence::weekly,420,4,none,0},baseTime)); // Wednesday
    CHECK(!due({true,15,0,none,Recurrence::weekly,420,2,none,0},baseTime));
    CHECK(due({true,15,0,none,Recurrence::interval,0,0,baseTime-minute,1},baseTime));
    CHECK(!due({true,15,0,baseTime,Recurrence::once,0,0,baseTime,0},baseTime));
    CHECK(due({true,15,baseTime,none,Recurrence::once,0,0,baseTime,0},baseTime));
    for (auto recurrence:{Recurrence::once,Recurrence::daily,Recurrence::weekly,Recurrence::interval}) {
        Fixture f;
        f.clock.s.wall=baseTime-250;
        f.tick();
        auto v=f.config("v",{15,15,windowSeconds,7200});
        v.schedule={true,15,0,none,recurrence,0,0,none,0};
        if (recurrence==Recurrence::once) v.schedule.at=baseTime;
        if (recurrence==Recurrence::daily) v.schedule.localMinute=420;
        if (recurrence==Recurrence::weekly) { v.schedule.localMinute=420; v.schedule.weekdays=4; }
        if (recurrence==Recurrence::interval) { v.schedule.at=baseTime; v.schedule.intervalMinutes=1; }
        f.apply(v); f.advance(250);
        CHECK(f.actuator.opens==1);
        f.tick(); f.advance(15*second);
        v.ref=id("paused"); v.schedule.enabled=false; f.apply(v);
        v.ref=id("resumed"); v.schedule.enabled=true; f.apply(v);
        f.reboot(false); f.tick();
        CHECK(f.actuator.opens==1);
        const auto e=f.offer();
        const auto* o=find(e,Kind::occurrence_decided);
        CHECK(o && o->outcome==Outcome::started && o->occurrence==baseTime);
    }
}
void missedAndRejectedNotReplayed() {
    Fixture f;
    auto v=f.config("a",{15,15,windowSeconds,7200});
    v.schedule={true,15,0,none,Recurrence::interval,0,0,baseTime+minute,1};
    f.apply(v);
    f.gap(61*second);
    CHECK(!f.actuator.opens && find(f.offer(),Kind::occurrence_decided,Reason::clock_jump));
    f.reboot(false); f.advance(58*second); f.actuator.pressStop(); f.advance(second);
    CHECK(!f.actuator.opens);
    f.actuator.releaseStop(); f.tick(); v.ref=id("new"); f.apply(v);
    CHECK(!f.actuator.opens);
    f.advance(minute);
    CHECK(f.actuator.opens==1);
}
void clockChangesDoNotMoveDeadlineOrRefill() {
    Fixture f;
    f.apply(f.config("a",{15,15,windowSeconds,15}));
    f.tick(true); f.advance(7*second);
    f.clock.s.wall+=4*60*minute;
    f.tick(); CHECK(f.c->snapshot().on);
    f.advance(8*second);
    CHECK(!f.c->snapshot().on);
    f.tick(true);
    CHECK(f.actuator.opens==1);
    const auto events=f.offer();
    const auto* end=find(events,Kind::session_ended);
    CHECK(end && end->outputMs==15000 && end->ended==none);
    f.clock.s.wall=baseTime;
    f.reboot(false); f.tick(true);
    CHECK(f.c->snapshot().ready && f.actuator.opens==1);
    // Civil time cannot move the carried accounting intervals backwards or
    // forwards. The already-used 15 s still prevent another 15/15 START.
    CHECK(find(f.offer(),Kind::request_rejected,Reason::rolling_limit));
}
void accountingAcrossReboot() {
    for (auto jump : {4*60*minute, -4*60*minute}) {
        Fixture f;
        f.apply(f.config("default"));
        f.tick(true); f.advance(900*second);
        CHECK(f.c->ledger().history.size()==1);
        const auto carried=f.c->ledger().history;
        const auto floor=f.c->ledger().accountingFloor;
        f.clock.s.wall+=jump;
        f.reboot();
        CHECK(f.c->ledger().accountingFloor==floor);
        CHECK(f.c->ledger().history.front().start==carried.front().start);
        f.clock.s.quality=Quality::synced; f.tick();
        CHECK(f.c->snapshot().ready);
        for (int i=0;i<7;++i) {
            f.tick(true); CHECK(f.c->snapshot().on); f.advance(900*second);
            f.offer(); f.ack(); // inspect the final refusal without the earlier 16-record batch hiding it
        }
        CHECK(f.actuator.opens==8); // carried15 + usable105 = default120
        f.tick(true);
        CHECK(f.actuator.opens==8 && find(f.offer(),Kind::request_rejected,Reason::rolling_limit));
    }
}
void uptimeAgesCarriedBudgetWithoutDowntimeCredit() {
    Fixture f;
    f.apply(f.config("tight",{15,15,windowSeconds,15}));
    f.tick(true); f.advance(15*second);
    f.clock.s.wall+=10*day;
    f.reboot(false);
    f.tick(true); CHECK(f.actuator.opens==1); // SNTP jump + reboot did not refill
    f.gap(windowSeconds*second-15001);
    f.tick(true); CHECK(f.actuator.opens==1);
    f.gap(15001);
    f.tick(true); CHECK(f.actuator.opens==2); // actual powered uptime now permits a new run
    f.advance(15*second);
    f.reboot(false); f.tick(true); CHECK(f.actuator.opens==2);
}
void actualDelayedDeadlineIsAccounted() {
    Fixture f;
    f.apply(f.config("exact",{1800,1800,windowSeconds,7200}));
    f.actuator.exactDeadline=false;
    f.tick(true);
    CHECK(f.c->snapshot().active.duration==1800);
    f.clock.elapse(1800*second+7);
    f.tick();
    CHECK(f.c->ledger().history.back().end-f.c->ledger().history.back().start==1800007);
    CHECK(!f.c->snapshot().on && !f.c->ledger().unknown);
    f.reboot(false);
    CHECK(f.c->ledger().history.back().end-f.c->ledger().history.back().start==1800007);
    f.tick(true); CHECK(f.c->snapshot().active.duration==1800);
}
void fillTo512(Fixture& f) {
    f.apply(f.config());
    for (int i=0;i<510;++i) f.tick(true,true);
    CHECK(f.c->queued()==511);
    f.stop("filler");
    CHECK(f.c->queued()==512);
}
void stopReceiptSurvivesFullQueueAckAndRetry() {
    Fixture f;
    fillTo512(f);
    auto events=f.offer();
    const auto e=f.c->snapshot().epoch;
    Exchange x;
    x.acked=f.sent;
    x.stops={{id("stop-full"),f.c->bootId(),f.clock.s.wall,true}};
    f.exchange(x);
    CHECK(f.c->snapshot().epoch==e+1 && f.c->queued()==497);
    CHECK(f.c->ledger().deferredStops.empty());
    x.acked.clear(); f.exchange(x);
    CHECK(f.c->snapshot().epoch==e+1 && f.c->queued()==497);
    bool seen=false;
    while (f.c->queued()) {
        events=f.offer();
        for (const auto& r:events) if (r.kind==Kind::stop_processed && r.request==id("stop-full")) {
            seen=true; CHECK(r.outcome==Outcome::already_off);
        }
        f.ack();
    }
    CHECK(seen);
}
void deferredStopRecoversAfterReboot() {
    Fixture f;
    fillTo512(f);
    const auto oldBoot=f.c->bootId();
    f.stop("recover");
    CHECK(f.c->queued()==512 && f.c->ledger().deferredStops.size()==1);
    f.reboot(false);
    CHECK(f.c->ledger().deferredStops.size()==1);
    f.offer(); f.ack();
    CHECK(f.c->ledger().deferredStops.empty() && f.c->queued()==497);
    Exchange retry;
    retry.stops={{id("recover"),oldBoot,f.clock.s.wall,true}};
    f.exchange(retry);
    CHECK(f.c->snapshot().epoch==0 && f.c->queued()==497);
    bool seen=false;
    while (f.c->queued()) {
        const auto events=f.offer();
        for (const auto& r:events) if (r.kind==Kind::stop_processed && r.request==id("recover")) {
            CHECK(r.boot==oldBoot && r.outcome==Outcome::already_off && r.occurred==baseTime);
            seen=true;
        }
        f.ack();
    }
    CHECK(seen);
}
void deferredStopJournalFailureIsNotProof() {
    Fixture f;
    fillTo512(f);
    const auto oldBoot=f.c->bootId();
    f.storage.failAt=f.storage.appendNumber+1;
    f.stop("failed-stop");
    CHECK(!f.c->snapshot().on && f.c->snapshot().blocked==Reason::storage_fault);
    f.reboot(false); f.offer(); f.ack();
    Exchange old;
    old.stops={{id("failed-stop"),oldBoot,f.clock.s.wall,true}};
    f.exchange(old);
    bool unconfirmed=false;
    while (f.c->queued()) {
        const auto events=f.offer();
        for (const auto& r:events) if (r.request==id("failed-stop")) {
            CHECK(r.kind==Kind::stop_processed && r.outcome==Outcome::unconfirmed);
            unconfirmed=true;
        }
        f.ack();
    }
    CHECK(unconfirmed);
}
void configRefBoundariesSurviveCompactAndBoot() {
    Fixture f;
    for (int n=1;n<=255;++n) {
        f.apply(f.config("cfg-"+std::to_string(n)));
        f.offer(); f.ack();
    }
    CHECK(f.c->ledger().configRefs.size()==255);
    f.apply(f.config("cfg-256")); f.offer(); f.ack();
    CHECK(f.c->ledger().configRefs.size()==256 && f.c->snapshot().applied==id("cfg-256"));
    f.apply(f.config("cfg-257"));
    CHECK(find(f.offer(),Kind::config_rejected,Reason::config_capacity));
    for (int n=0;n<1000;++n) { f.tick(true,true); if (n%12==11) { f.offer(); f.ack(); } }
    CHECK(f.storage.replacements>0);
    f.reboot(false);
    f.apply(f.config("cfg-257"));
    CHECK(f.c->ledger().configRefs.size()==256 && f.c->snapshot().applied==id("cfg-256"));
    f.apply(f.config("cfg-1"));
    CHECK(find(f.offer(),Kind::config_rejected,Reason::stale_config));
}
void maximumLedgerAndDeferredReceiptsFit() {
    Memory store;
    Journal journal(store);
    Ledger d;
    CHECK(journal.restore(d));
    d.applied.ref=id(std::string(128,'c'));
    d.accountingFloor=100000;
    for (size_t n=0;n<intervalMax;++n) d.history.push_back({Ms(2*n),Ms(2*n+1)});
    for (size_t n=0;n<configHistoryMax-1;++n) d.configRefs.push_back(n);
    d.configRefs.push_back(idFingerprint(d.applied.ref.view()));
    for (size_t n=0;n<deferredStopMax;++n) {
        Event e;
        e.boot.set(std::string(64,'b'));
        e.config=d.applied.ref;
        auto name=std::string(125,'r')+std::to_string(n);
        e.request=id(name);
        e.kind=Kind::stop_processed; e.source=Source::remote; e.outcome=Outcome::already_off;
        d.deferredStops.push_back(e);
    }
    const auto before=journal.bytes();
    CHECK(journal.commit(&d) && journal.bytes()-before<=Journal::maxFrame);
    Journal restored(store); Ledger read;
    CHECK(restored.restore(read) && read.history.size()==intervalMax &&
          read.configRefs.size()==configHistoryMax && read.deferredStops.size()==deferredStopMax);
    CHECK(restored.prepare(5));
}
void stopUsesMemoryProofBeforeStorage() {
    Fixture f;f.apply(f.config());f.stop("old-stop");
    BootId other;CHECK(other.set("old-boot"));
    Exchange stale;
    stale.stops={{id("new-stop"),other,f.clock.s.wall,true}};
    f.exchange(stale);
    CHECK(find(f.offer(),Kind::stop_processed,Reason::stale_boot));
    f.tick(true);
    CHECK(f.c->snapshot().on);
    f.stop("old-stop"); // duplicate must not cancel a newer session
    CHECK(f.c->snapshot().on);
    f.storage.beforeRead=[&]{CHECK(!f.actuator.d.on);};
    f.stop("new-stop"); // every subsequent storage read is after immediate CLOSE
    f.storage.beforeRead={};
    CHECK(!f.c->snapshot().on);
}
void boundedQueueCompactionAndAck() {
    Fixture f;
    f.apply(f.config("a",{15,15,windowSeconds,7200}));
    for (int i=0;i<4000;++i) { f.tick(true,true); if (i%12==11) { f.offer(); f.ack(); } }
    CHECK(f.storage.replacements>0);
    f.offer(); f.ack();
    for (int i=0;i<600;++i) f.tick(true,true);
    CHECK(f.c->queued()==511 && f.c->snapshot().blocked==Reason::report_capacity);
    CHECK(f.c->evidenceGaps()>0 && !f.actuator.opens);
    f.stop();
    CHECK(f.c->queued()==512);
    auto pending=f.offer(); CHECK(pending.size()==16);
    f.reboot(false);
    CHECK(f.c->queued()==512);
    auto recovered=f.offer();
    CHECK(eventId(recovered.front())==eventId(pending.front()) && recovered.front().sequence==pending.front().sequence);
    f.ack();
    CHECK(f.c->queued()==496 && f.c->snapshot().ready);
    f.tick(true); f.advance(15*second);
    CHECK(f.actuator.opens==1 && f.c->ledger().history.size()==1);
}
void compactFailureKeepsOriginal() {
    Memory store;
    Journal j(store);
    Ledger d; CHECK(j.restore(d));
    for (int i=0;i<510;++i) {
        Event e; e.boot.set("boot-compaction"); e.kind=Kind::fault; e.reason=Reason::storage_fault; e.sequence=++d.sequence;
        e.config=id(std::string(128,'x')); e.request=e.config; e.session=e.config;
        CHECK(j.commit(&d,{e}));
    }
    const auto before=store.live;
    store.failReplace=true;
    // Demand enough space that the compact/rename path is required.
    CHECK(!j.prepare(12) && store.live==before);
    Journal again(store); Ledger record;
    CHECK(again.restore(record) && again.queued()==510);
}
void ackOnlySentAndPriorityReceipt() {
    Fixture f;
    f.apply(f.config());
    for (int i=0;i<45;++i) f.tick(true,true);
    auto remote=f.intent();
    f.candidate(remote);
    auto e=f.offer();
    CHECK(e.size()==batchMax && e.back().kind==Kind::request_received);
    Exchange x;
    Event forged=e.front(); forged.sequence+=17;
    x.acked={eventId(forged)};
    const auto count=f.c->queued(); f.exchange(x);
    CHECK(f.c->queued()==count);
    f.grant(remote); CHECK(f.actuator.opens==1);
}
void maintenanceGate() {
    Fixture f;
    f.apply(f.config());
    f.tick(true);
    CHECK(!f.c->enterMaintenance() && f.c->snapshot().on);
    f.stop();
    const auto pending=f.intent();
    f.candidate(pending); f.offer();
    CHECK(f.c->enterMaintenance() && !f.c->snapshot().on);
    f.gap(100*second); f.tick(true); f.grant(pending);
    CHECK(f.actuator.opens==1 && !f.c->ledger().pending.receivedSequence);
    f.c->leaveMaintenance(); f.grant(pending);
    CHECK(f.actuator.opens==1);
    f.tick(true);
    CHECK(f.actuator.opens==2);
}
void syncCadence() {
    SyncPolicy s;
    CHECK(!s.due(0,false,1) && s.due(10,true,1)); s.sent(1);
    s.ok(10,false,false,-1);
    CHECK(!s.due(15009,true,1) && s.due(100,true,2) && s.due(15010,true,1));
    s.ok(200,true,false,-1); CHECK(s.next()==5200);
    s.ok(210,true,true,-1); CHECK(s.due(210,true,1));
    s.transient(300,0); CHECK(s.next()==5300 && !s.due(301,true,2));
    s.transient(5300,3); CHECK(s.next()==20303);
    s.transient(20303,0,70); CHECK(s.next()==90303);
    CHECK(!s.due(30000,false,2) && !s.due(30001,true,2));
    s.transient(90303,1000); CHECK(s.next()==151303);
    s.permanent(); CHECK(!s.due(200000,true,3));
}
void timeCodec() {
    char t[32]; Ms v=0;
    CHECK(parseUtc("2026-11-18T00:00:00Z",v) && v==baseTime);
    CHECK(parseUtc("2028-02-29T23:59:58.123456+00:00",v));
    formatUtc(v,t); CHECK(std::string_view(t)=="2028-02-29T23:59:58.123Z");
    formatUtc(maxUtc,t); CHECK(std::string_view(t)=="9999-12-31T23:59:59.999Z");
    CHECK(!parseUtc("2026-02-29T00:00:00Z",v));
    CHECK(!parseUtc("2026-11-18T07:00:00+07:00",v));
    CHECK(!parseUtc("2026-11-18T00:00:00.0",v));
}
void wireVectors(const char* path) {
    std::ifstream f(path);
    JsonDocument vectors;
    CHECK(f && !deserializeJson(vectors,f));
    unsigned cases=0;
    for (JsonObjectConst v:vectors.as<JsonArrayConst>()) {
        JsonObjectConst d=v["document"];
        if (!d["server_time"].is<const char*>()) continue;
        std::string line;
        serializeJson(d,line);
        Exchange x;
        const auto error=decodeExchange(line,x);
        const auto name=v["name"].as<std::string_view>();
        if (name=="wrong_protocol") CHECK(error==WireError::protocol_version);
        else if (name=="wrong_window") CHECK(error==WireError::none && x.hasConfig && x.invalidConfig);
        else { CHECK(error==WireError::none); if (name=="desired") CHECK(x.hasConfig && !x.invalidConfig); }
        CHECK(decodeExchange(line+" false",x)==WireError::syntax);
        ++cases;
    }
    CHECK(cases==5);
}
int main(int argc,char** argv) {
    int pass=0,fail=0;
    auto test=[&](const char* name,auto f) {
        try { f(); ++pass; std::cout<<"PASS "<<name<<"\n"; }
        catch (const std::exception& e) { ++fail; std::cerr<<"FAIL "<<name<<": "<<e.what()<<"\n"; }
    };
#define TEST(f) test(#f,f)
    if(argc==3&&std::string_view(argv[2])=="--recovery-delta") {
        TEST(stopBeatsGrantAndLocal);TEST(remoteContextAndReboot);TEST(unknownAcrossSourcesAndAck);
        TEST(persistBeforeOutput);TEST(failureAfterOutputAndTerminal);TEST(damageDuringSession);
        TEST(corruptionBeforeAdmissionAndMissingStore);TEST(uncertainClosure);
        TEST(accountingAcrossReboot);TEST(uptimeAgesCarriedBudgetWithoutDowntimeCredit);
        TEST(stopReceiptSurvivesFullQueueAckAndRetry);TEST(deferredStopRecoversAfterReboot);
        TEST(maximumLedgerAndDeferredReceiptsFit);TEST(maintenanceGate);TEST(syncCadence);
        test("wireVectors",[&]{wireVectors(argv[1]);});
        std::cout<<"TOTAL "<<pass<<" PASS, "<<fail<<" FAIL\n";return fail!=0;
    }
    TEST(configAndBoot); TEST(defaultsAndDeadline); TEST(configInvalidConflictAndStale); TEST(applyCutover);
    TEST(exactBudgetOracle); TEST(allSourcesOneBudget); TEST(sourceBusyIsNotFifo);
    TEST(stopWithoutReadinessAndDedup); TEST(stopBeatsGrantAndLocal); TEST(stopBetweenReservationAndOutput);
    TEST(remoteHandoffAndStableReports); TEST(expiryAndMismatchedGrant); TEST(finalOutputTimeFence);
    TEST(wallRollbackCannotRenewHandoff);
    TEST(remoteContextAndReboot); TEST(unknownAcrossSourcesAndAck); TEST(persistBeforeOutput);
    TEST(failureAfterOutputAndTerminal); TEST(damageDuringSession); TEST(corruptionBeforeAdmissionAndMissingStore);
    TEST(uncertainClosure); TEST(schedulePatternsAndCutover); TEST(missedAndRejectedNotReplayed);
    TEST(clockChangesDoNotMoveDeadlineOrRefill); TEST(boundedQueueCompactionAndAck); TEST(compactFailureKeepsOriginal);
    TEST(accountingAcrossReboot); TEST(uptimeAgesCarriedBudgetWithoutDowntimeCredit); TEST(actualDelayedDeadlineIsAccounted);
    TEST(stopReceiptSurvivesFullQueueAckAndRetry); TEST(deferredStopRecoversAfterReboot);
    TEST(deferredStopJournalFailureIsNotProof); TEST(configRefBoundariesSurviveCompactAndBoot);
    TEST(maximumLedgerAndDeferredReceiptsFit);
    TEST(stopUsesMemoryProofBeforeStorage);
    TEST(ackOnlySentAndPriorityReceipt); TEST(maintenanceGate); TEST(syncCadence); TEST(timeCodec);
    test("wireVectors",[&] { CHECK(argc==2); wireVectors(argv[1]); });
    std::cout<<"TOTAL "<<pass<<" PASS, "<<fail<<" FAIL\n";
    return fail!=0;
}
