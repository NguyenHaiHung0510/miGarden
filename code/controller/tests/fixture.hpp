#pragma once
#include "controller.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace mg::test {
inline constexpr Ms baseTime=1794960000000LL; // 2026-11-18T00:00:00Z
inline Id id(std::string_view s) { Id v; v.set(s); return v; }
struct Clock : ClockPort {
    Sample s{0,baseTime,Quality::synced};
    Sample now() override { return s; }
    void elapse(Ms ms) { s.mono+=ms; s.wall+=ms; }
};
struct Memory : LogFile {
    std::vector<uint8_t> live=Journal::genesis(), draft;
    bool readable=true, failReplace=false;
    int appendNumber=0, failAt=0, replacements=0;
    std::function<void()> afterAppend;
    int64_t size() override { return readable?int64_t(live.size()):-1; }
    bool read(uint32_t at,uint8_t* out,size_t n) override {
        if (!readable || at>live.size() || n>live.size()-at) return false;
        std::copy_n(live.data()+at,n,out);
        return true;
    }
    bool append(const uint8_t* p,size_t n) override {
        if (++appendNumber==failAt) return false;
        live.insert(live.end(),p,p+n);
        if (afterAppend) afterAppend();
        return true;
    }
    bool beginReplacement() override { draft.clear(); return true; }
    bool writeReplacement(const uint8_t* p,size_t n) override { draft.insert(draft.end(),p,p+n); return true; }
    bool publishReplacement() override {
        if (failReplace) return false;
        live.swap(draft); draft.clear(); ++replacements; return true;
    }
    void discardReplacement() override { draft.clear(); }
};
struct Actuator : OutputPort {
    explicit Actuator(Clock& clock) : clock(clock) {}
    Clock& clock;
    Drive d;
    Ms deadline=none;
    unsigned opens=0, closes=0;
    bool exactDeadline=true;
    std::function<void()> beforeOpen;
    void poll() {
        if (d.on && clock.s.mono>=deadline) {
            d.on=false; d.offAt=exactDeadline?deadline:clock.s.mono; d.offReason=Reason::deadline; ++closes;
        }
    }
    Drive read() override { poll(); return d; }
    bool open(Ms duration,Ms latest,uint32_t stops,Drive& actual) override {
        if (beforeOpen) beforeOpen();
        if (!d.usable || d.stopHeld || d.on || d.stops!=stops || clock.s.mono>=latest) return false;
        d.on=true; d.onAt=clock.s.mono; d.offAt=none; d.offReason=Reason::none;
        deadline=d.onAt+duration; actual=d; ++opens; return true;
    }
    Drive close(Reason why) override {
        poll();
        if (d.on) { d.on=false; d.offAt=clock.s.mono; d.offReason=why; ++closes; }
        return d;
    }
    void pressStop() { if (!d.stopHeld) { d.stopHeld=true; ++d.stops; } close(Reason::stopped); }
    void releaseStop() { d.stopHeld=false; }
};
struct Fixture {
    Clock clock;
    Memory storage;
    Actuator actuator{clock};
    std::unique_ptr<Journal> journal;
    std::unique_ptr<Controller> c;
    unsigned boots=0;
    std::vector<EventId> sent;
    void boot() {
        actuator.close(Reason::interlock);
        c.reset();
        journal=std::make_unique<Journal>(storage);
        BootId b; b.set("boot-"+std::to_string(++boots));
        c=std::make_unique<Controller>(clock,actuator,*journal,b);
        c->boot(); sent.clear();
        c->tick();
    }
    Fixture() { boot(); }
    void reboot(bool unknownClock=true) {
        clock.s.mono=0; actuator.d={};
        if (unknownClock) clock.s.quality=Quality::unknown;
        boot();
    }
    void tick(bool start=false,bool busy=false) { c->tick(start,busy); }
    void exchange(const Exchange& x) { c->tick(false,false,&x,sent); }
    void gap(Ms ms) { clock.elapse(ms); tick(); }
    void advance(Ms ms) {
        while (ms>0) {
            const auto n=std::min(ms,Ms{50}); clock.elapse(n); tick(); ms-=n;
        }
    }
    Config config(std::string_view ref="cfg-1",Policy p={}) {
        Config v; v.ref=id(ref); v.policy=p; v.schedule.duration=std::min(p.session,900); return v;
    }
    void apply(const Config& v) { Exchange x; x.hasConfig=true; x.config=v; exchange(x); }
    std::vector<Event> offer() {
        auto e=c->reports(); sent.clear();
        for (auto& r:e) sent.push_back(eventId(r));
        return e;
    }
    void ack() { Exchange x; x.acked=sent; exchange(x); }
    Intent intent(std::string_view req="remote-1",int seconds=15) {
        const auto s=c->snapshot();
        return {id(req),s.applied,c->bootId(),s.epoch,s.time.wall,s.time.wall+ttlSeconds*second,seconds,true};
    }
    void candidate(const Intent& i) { Exchange x; x.candidates={i}; exchange(x); }
    void grant(const Intent& i) { Exchange x; x.grants={{i,id("grant-1")}}; exchange(x); }
    void stop(std::string_view req="stop-1") {
        Exchange x; x.stops={{id(req),c->bootId(),clock.s.wall,true}}; exchange(x);
    }
};
inline const Event* find(const std::vector<Event>& e,Kind kind,Reason reason=Reason::none) {
    for (const auto& r:e) if (r.kind==kind && (reason==Reason::none || r.reason==reason)) return &r;
    return nullptr;
}
}
