#include "controller.hpp"
#include <algorithm>
#include <cstdio>
#include <utility>

namespace mg {
namespace {
constexpr size_t remoteIdsPerBoot = 4096;
constexpr size_t stopsPerBoot = 128;
bool has(const std::vector<uint64_t>& xs, uint64_t id) {
    return std::find(xs.begin(),xs.end(),id)!=xs.end();
}
Ms absMs(Ms n) { return n < 0 ? -n : n; }
}
Controller::Controller(ClockPort& c, OutputPort& o, Journal& j, BootId boot)
    : clock_(c), output_(o), journal_(j), bootId_(boot) {}

void Controller::boot() {
    output_.close(Reason::interlock);
    // Do not consume a held-at-boot STOP; tick takes it as a new fence.
    auto d = output_.read();
    observedStops_ = d.stopHeld ? d.stops - 1 : d.stops;
    if (bootId_.empty() || !journal_.restore(state_)) { breakStorage(); return; }
    const auto bootTime=clock_.now();
    if (state_.accountingFloor!=none && bootTime.mono>=0) {
        // Existing v1 ledgers already store this floor. Downtime cannot be
        // proved by SNTP after a reboot: retain all carried intervals at the
        // last durable point, then age them only by this boot's elapsed time.
        anchor_={bootTime.mono,state_.accountingFloor,Quality::holdover};
    }
    if (state_.session.phase==Session::Phase::armed || state_.session.phase==Session::Phase::running ||
        state_.pending.receivedSequence) {
        Ledger next=state_;
        const Sample now=clock_.now();
        std::vector<Event> records;
        if (next.session.phase==Session::Phase::armed || next.session.phase==Session::Phase::running) {
            const auto original=next.session;
            next.unknown=true;
            possible_=true;
            next.session.phase=Session::Phase::unresolved;
            if (journal_.queued() < backlogMax) {
                if (original.phase==Session::Phase::running) {
                    auto e=fromSession(next,Kind::session_ended,now,original);
                    e.reason=Reason::reboot_interrupted; e.outcome=Outcome::unknown;
                    records.push_back(e);
                } else {
                    auto e=fromSession(next,Kind::fault,now,original);
                    e.session={}; // armed reservation does not prove a session started
                    e.reason=Reason::outcome_unknown;
                    records.push_back(e);
                }
            } else ++gaps_;
        }
        if (next.pending.receivedSequence) {
            if (journal_.queued()+records.size()<backlogMax) cancelPending(next,records,now,Reason::stale_boot);
            else { next.pending={}; ++gaps_; }
        }
        save(std::move(next),records,true);
    }
}

Ms Controller::accounting(Sample s) const {
    if (!anchor_.usable() || s.mono<anchor_.mono) return none;
    const Ms elapsed=s.mono-anchor_.mono;
    return elapsed<=maxUtc-anchor_.wall ? anchor_.wall+elapsed : none;
}

bool Controller::observe(Sample s) {
    if (s.mono < 0 || s.mono < lastMonotonic_) {
        if (running_) {
            auto d=output_.close(Reason::time_unknown);
            Ledger next=state_; std::vector<Event> reports;
            finish(next,reports,{},d,Reason::time_unknown);
            save(std::move(next),reports,true);
        }
        possible_=true;
        return false;
    }
    lastMonotonic_=s.mono;
    if (!anchor_.usable() && s.usable()) anchor_=s; // virgin safety timeline only
    return true;
}

Reason Controller::gate(Sample now, Drive d, bool ignoreRunning) const {
    if (maintenance_) return Reason::maintenance;
    if (d.stopHeld) return Reason::stop_held;
    if ((!ignoreRunning && running_) || d.on) return Reason::busy;
    if (state_.unknown || possible_) return Reason::outcome_unknown;
    if (broken_ || !journal_.healthy()) return Reason::storage_fault;
    if (remoteStopCapacity_) return Reason::remote_capacity;
    if (!d.usable) return Reason::interlock;
    if (state_.applied.ref.empty()) return Reason::config_missing;
    if (!now.usable() || accounting(now)<state_.accountingFloor || accounting(now)==none) return Reason::time_unknown;
    if (journal_.queued()+privilegedRoom>backlogMax) return Reason::report_capacity;
    const auto oldest=accounting(now)-windowSeconds*second;
    if (state_.history.size()>=intervalMax && state_.history.front().end>oldest) return Reason::interval_capacity;
    return Reason::none;
}

Reason Controller::remote(const Intent& i, Sample now) const {
    if (!i.target) return Reason::wrong_target;
    if (i.boot!=bootId_) return Reason::stale_boot;
    if (i.epoch!=epoch_) return Reason::stale_epoch;
    if (i.config!=state_.applied.ref || i.config.empty()) return Reason::context_changed;
    if (!now.usable()) return Reason::time_unknown;
    if (i.confirmed<0 || i.expires-i.confirmed!=ttlSeconds*second || now.wall<i.confirmed) return Reason::invalid_ttl;
    if (now.wall>=i.expires) return Reason::expired;
    if (state_.pending.receivedSequence && state_.pending.intent.request==i.request &&
        (pendingExpires_==none || now.mono>=pendingExpires_)) return Reason::expired;
    if (!durationValid(i.duration) || i.duration>state_.applied.policy.session) return Reason::invalid_duration;
    return Reason::none;
}

Event Controller::make(Ledger& l, Kind kind, Sample now, Source source, Reason reason) {
    Event e;
    e.boot=bootId_; e.sequence=++l.sequence; e.kind=kind; e.source=source; e.reason=reason;
    if (now.usable()) { e.occurred=now.wall; e.quality=now.quality; }
    e.config=l.applied.ref;
    return e;
}
Event Controller::fromSession(Ledger& l, Kind kind, Sample now, const Session& s) {
    auto e=make(l,kind,now,s.source);
    e.config=s.config; e.request=s.request; e.session=s.id; e.started=s.started;
    e.occurrence=s.occurrence; e.plannedSeconds=s.duration;
    return e;
}
Event Controller::occurrence(Ledger& l, Sample now, Ms at, Outcome outcome, Reason why,
                             const Id& request, const Id& session) {
    auto e=make(l,Kind::occurrence_decided,now,Source::schedule,why);
    e.occurrence=at; e.outcome=outcome; e.request=request; e.session=session;
    return e;
}
Id Controller::newId(const char* prefix) {
    char buf[idMax+1];
    std::snprintf(buf,sizeof buf,"%s.%s.%llu",prefix,bootId_.c_str(),
                  static_cast<unsigned long long>(++localCounter_));
    Id id; id.set(buf); return id;
}

void Controller::breakStorage() {
    if (!broken_) { ++change_; ++gaps_; }
    broken_=true;
    if (running_ || output_.read().on || state_.session.phase==Session::Phase::armed ||
        state_.session.phase==Session::Phase::running) {
        output_.close(Reason::storage_fault);
        possible_=true; running_=false;
    }
}
bool Controller::save(Ledger&& next, const std::vector<Event>& reports, bool critical) {
    if (broken_ || !journal_.healthy()) { breakStorage(); return false; }
    if (journal_.queued()+reports.size()>backlogMax ||
        (!critical && journal_.queued()+reports.size()>backlogMax-(running_ ? 3 : 1))) {
        ++gaps_; ++change_;
        if (critical) breakStorage();
        return false;
    }
    if ((!running_ && !journal_.prepare(2)) ||
        (running_ && !critical && journal_.bytes()>Journal::maxLog-3*Journal::maxFrame)) {
        if (!journal_.healthy()) breakStorage(); else { ++gaps_; ++change_; }
        return false;
    }
    if (!journal_.commit(&next,reports)) { breakStorage(); return false; }
    state_=std::move(next);
    if (!reports.empty()) ++change_;
    return true;
}

void Controller::cancelPending(Ledger& l, std::vector<Event>& events, Sample now, Reason why) {
    if (!l.pending.receivedSequence) return;
    auto e=make(l,Kind::request_rejected,now,Source::remote,why);
    e.config=l.pending.intent.config; e.request=l.pending.intent.request;
    events.push_back(e);
    l.pending={};
    offeredSequence_=0;
    pendingExpires_=none;
}

void Controller::finish(Ledger& l, std::vector<Event>& out, Sample now, Drive drive, Reason why) {
    if (!running_) return;
    const auto s=l.session;
    const Ms elapsed=(drive.onAt==monoStart_ && !drive.on && drive.offAt>=monoStart_)
        ? drive.offAt-monoStart_ : none;
    auto e=fromSession(l,Kind::session_ended,now,s);
    if (elapsed<0 || s.accountedAt>maxUtc-elapsed || why==Reason::unsafe_output) {
        l.unknown=true; possible_=true; l.session.phase=Session::Phase::unresolved;
        e.reason=why; e.outcome=Outcome::unknown;
        e.occurred=none; e.quality=Quality::unknown;
    } else {
        e.outputMs=elapsed;
        const Ms end=s.accountedAt+elapsed;
        l.accountingFloor=std::max(l.accountingFloor,end);
        if (elapsed>0) {
            if (!l.history.empty() && l.history.back().end==s.accountedAt) l.history.back().end=end;
            else l.history.push_back({s.accountedAt,end});
        }
        // Keep a reliable elapsed duration on a civil clock correction. Leave
        // civil end null if the sample disagrees with the start/elapsed anchor.
        if (now.usable() && now.mono>=drive.offAt && s.started<=maxUtc-elapsed) {
            const auto observedEnd=now.wall-(now.mono-drive.offAt);
            if (observedEnd>=s.started && absMs(observedEnd-(s.started+elapsed))<=second) {
                e.ended=observedEnd;
                e.occurred=observedEnd;
            } else { e.occurred=none; e.quality=Quality::unknown; }
        }
        e.reason=why;
        e.outcome=why==Reason::deadline ? Outcome::completed :
            (why==Reason::stopped || why==Reason::remote_stop ? Outcome::stopped : Outcome::fault);
        l.session={};
    }
    out.push_back(e);
    running_=false; monoStart_=deadline_=none;
}

void Controller::handleStop(Sample now, Source source, const Id& request, bool stale) {
    const auto before=output_.read();
    const bool stoppedByInput=running_ && before.offReason==Reason::stopped;
    const auto why=source==Source::remote ? Reason::remote_stop : Reason::stopped;
    const auto after=stale ? before : output_.close(why); // no readiness/clock/storage gate
    if (!stale) {
        if (epoch_==UINT32_MAX) possible_=true; else ++epoch_;
        ++change_;
    }
    if (broken_ || !journal_.healthy()) { ++gaps_; breakStorage(); return; }
    Ledger next=state_;
    std::vector<Event> events;
    if (!stale) {
        finish(next,events,now,after,after.offReason==Reason::deadline ? Reason::deadline : why);
        if (next.pending.receivedSequence) {
            if (journal_.queued()+events.size()+1<backlogMax) cancelPending(next,events,now,why);
            else { next.pending={}; offeredSequence_=0; pendingExpires_=none; ++gaps_; }
        }
    }
    if (journal_.queued()+events.size()<backlogMax) {
        auto e=make(next,Kind::stop_processed,now,source,stale ? Reason::stale_boot : Reason::none);
        e.request=request;
        e.outcome=stale || after.on ? Outcome::unconfirmed :
            (before.on || stoppedByInput ? Outcome::close_requested : Outcome::already_off);
        events.push_back(e);
    } else if (source==Source::remote && !stale && next.deferredStops.size()<deferredStopMax) {
        auto e=make(next,Kind::stop_processed,now,source);
        --next.sequence; e.sequence=0; // not an exposed report/event ID until it fits the queue
        e.request=request;
        e.outcome=after.on ? Outcome::unconfirmed :
            (before.on || stoppedByInput ? Outcome::close_requested : Outcome::already_off);
        next.deferredStops.push_back(e);
    } else { ++gaps_; if (!stale) remoteStopCapacity_=true; }
    // Do not consume the last terminal slot reporting a stale STOP while active.
    if (!stale && after.on) {
        possible_=true; next.unknown=true;
    }
    if (!events.empty() || state_.pending.receivedSequence!=next.pending.receivedSequence ||
        state_.deferredStops.size()!=next.deferredStops.size())
        save(std::move(next),events,!stale);
}
void Controller::flushStopReceipts() {
    if (broken_ || !journal_.healthy() || state_.deferredStops.empty()) return;
    const auto keep=running_ ? size_t{3} : size_t{0};
    const auto available=backlogMax-journal_.queued();
    if (available<=keep) return;
    const auto count=std::min({available-keep,state_.deferredStops.size(),batchMax});
    Ledger next=state_;
    std::vector<Event> out;
    for (size_t n=0;n<count;++n) {
        auto e=next.deferredStops[n];
        e.sequence=++next.sequence;
        out.push_back(e); // keep original processing boot/time/outcome, no new physical claim
    }
    next.deferredStops.erase(next.deferredStops.begin(),next.deferredStops.begin()+count);
    save(std::move(next),out,true);
}

void Controller::rejected(Sample now, Source source, Reason why, const Id& request, const Id& ref) {
    if (broken_ || !journal_.healthy()) return;
    Ledger d=state_;
    auto e=make(d,Kind::request_rejected,now,source,why); e.request=request;
    if (!ref.empty()) e.config=ref;
    save(std::move(d),{e});
}

void Controller::apply(const Exchange& x, Sample now) {
    if (!x.hasConfig || running_ || maintenance_ || broken_ || !journal_.healthy()) return;
    const auto& c=x.config;
    Reason why=Reason::none;
    if (c.ref.empty()) return; // cannot identify the desired bundle; diagnostic at wire boundary
    if (x.invalidConfig || !valid(c)) why=Reason::invalid_config;
    else if (c.ref==state_.applied.ref) {
        if (c==state_.applied) return;
        why=Reason::config_conflict;
    } else if (has(state_.configRefs,idFingerprint(c.ref.view()))) why=Reason::stale_config;
    else if (state_.configRefs.size()==configHistoryMax) why=Reason::config_capacity;
    Ledger d=state_; std::vector<Event> reports;
    if (why!=Reason::none) {
        if (lastRejectedConfig_==c.ref && lastConfigReason_==why) return;
        auto e=make(d,Kind::config_rejected,now,Source::none,why); e.config=c.ref;
        if (save(std::move(d),{e})) { lastRejectedConfig_=c.ref; lastConfigReason_=why; }
        return;
    }
    if (d.pending.receivedSequence) cancelPending(d,reports,now,Reason::context_changed);
    d.applied=c;
    d.configRefs.push_back(idFingerprint(c.ref.view()));
    reports.push_back(make(d,Kind::config_applied,now));
    if (now.usable()) {
        const auto slot=slotOf(now.wall);
        if (slot>d.scheduleFloor && due(c.schedule,slot)) {
            reports.push_back(occurrence(d,now,slot,Outcome::skipped,Reason::context_changed));
        }
        d.scheduleFloor=std::max(d.scheduleFloor,slot);
    }
    save(std::move(d),reports);
}

void Controller::scheduled(Sample now, bool crossed, Reason simultaneous) {
    if (!now.usable() || broken_ || !journal_.healthy() || state_.applied.ref.empty() || maintenance_) return;
    const auto at=slotOf(now.wall);
    if (at<=state_.scheduleFloor || !due(state_.applied.schedule,at)) return;
    const auto why=simultaneous!=Reason::none ? simultaneous :
        (!crossed ? Reason::missed_boundary : gate(now,output_.read()));
    if (why==Reason::none) { start(Source::schedule,newId("sc"),state_.applied.schedule.duration,at); return; }
    Ledger d=state_;
    d.scheduleFloor=at;
    auto rec=occurrence(d,now,at,crossed ? Outcome::rejected : Outcome::skipped,why);
    if (journal_.queued()>=backlogMax-(running_ ? 3 : 1)) {
        // Preserve no-double even when no audit record fits. The gap is visible.
        ++gaps_; ++change_;
        save(std::move(d),{});
    } else save(std::move(d),{rec});
}

void Controller::local(Sample now, Reason simultaneous) {
    const auto request=newId("lc");
    const auto why=simultaneous!=Reason::none ? simultaneous : gate(now,output_.read());
    if (why!=Reason::none) { rejected(now,Source::local_button,why,request); return; }
    start(Source::local_button,request,state_.applied.policy.manual,none);
}

void Controller::candidate(const Intent& i, Sample now, Reason simultaneous) {
    if (i.request.empty() || !i.target) return;
    const auto h=idFingerprint(i.request.view());
    if (state_.pending.receivedSequence && i.request==state_.pending.intent.request) {
        if (!(i==state_.pending.intent)) {
            Ledger d=state_; std::vector<Event> e; cancelPending(d,e,now,Reason::intent_conflict);
            save(std::move(d),e);
        }
        return;
    }
    if (has(startSeen_,h)) return;
    if (startSeen_.size()>=remoteIdsPerBoot) { ++gaps_; return; }
    startSeen_.push_back(h);
    Reason why=remote(i,now);
    if (why==Reason::none) why=simultaneous;
    if (why==Reason::none) why=state_.pending.receivedSequence ? Reason::busy : gate(now,output_.read());
    if (why!=Reason::none) { rejected(now,Source::remote,why,i.request,i.config); return; }
    Ledger d=state_;
    auto e=make(d,Kind::request_received,now,Source::remote); e.request=i.request;
    d.pending={i,e.sequence};
    if (save(std::move(d),{e})) pendingExpires_=now.mono+i.expires-now.wall;
}

void Controller::grant(const Grant& g, Sample now, Reason simultaneous) {
    const auto& i=g.start;
    if (!i.target || g.id.empty() || !state_.pending.receivedSequence ||
        i.request!=state_.pending.intent.request || offeredSequence_!=state_.pending.receivedSequence) return;
    Reason why=!(i==state_.pending.intent) ? Reason::intent_conflict : remote(i,now);
    if (why==Reason::none) why=simultaneous;
    if (why==Reason::none) why=gate(now,output_.read());
    if (why!=Reason::none) {
        Ledger d=state_; std::vector<Event> e; cancelPending(d,e,now,why); save(std::move(d),e);
        return;
    }
    start(Source::remote,i.request,i.duration,none,&i);
}

void Controller::start(Source source, Id request, int32_t duration, Ms at, const Intent* i) {
    const auto t=clock_.now(); const auto d0=output_.read();
    const auto handoffDeadline=pendingExpires_;
    Reason why=gate(t,d0);
    if (why==Reason::none && i) why=remote(*i,t);
    if (why==Reason::none && (!durationValid(duration) || duration>state_.applied.policy.session))
        why=Reason::invalid_duration;
    Ledger next=state_;
    std::vector<Event> reports;
    if (at!=none) next.scheduleFloor=std::max(next.scheduleFloor,at);
    const Ms stamp=accounting(t), length=duration*second;
    if (why==Reason::none) {
        const auto end=stamp-windowSeconds*second;
        auto used=std::find_if(next.history.begin(),next.history.end(),[&](const Range& r){return r.end>end;});
        next.history.erase(next.history.begin(),used);
        if (stamp>maxUtc-length || rollingPeak(next.history,{stamp,stamp+length})>next.applied.policy.cumulative*second)
            why=Reason::rolling_limit;
        else if (next.history.size()>=intervalMax) why=Reason::interval_capacity;
    }
    auto disposition=[&](Ledger& d, std::vector<Event>& events, Sample when, Reason reason) {
        if (source==Source::schedule) events.push_back(occurrence(d,when,at,Outcome::rejected,reason,request));
        else { auto e=make(d,Kind::request_rejected,when,source,reason); e.request=request; events.push_back(e); }
    };
    if (why!=Reason::none) {
        if (i) { next.pending={}; offeredSequence_=0; pendingExpires_=none; }
        disposition(next,reports,t,why);
        save(std::move(next),reports);
        return;
    }
    if (!journal_.verifyTail() || !journal_.prepare(5)) { breakStorage(); return; }
    if (i) { next.pending={}; offeredSequence_=0; pendingExpires_=none; }
    else cancelPending(next,reports,t,Reason::busy);
    next.accountingFloor=std::max(next.accountingFloor,stamp);
    next.session={newId("s"),request,next.applied.ref,source,duration,at,none,stamp,Session::Phase::armed};
    if (!save(std::move(next),reports,true)) return;
    if (!journal_.verifyTail()) { breakStorage(); return; }

    const auto f=clock_.now();
    const auto attempt=output_.read();
    why=gate(f,attempt);
    if (why==Reason::none && i) {
        why=remote(*i,f);
        if (why==Reason::none && (handoffDeadline==none || f.mono>=handoffDeadline)) why=Reason::expired;
    }
    if (why==Reason::none && (attempt.stops!=d0.stops || attempt.stops!=observedStops_))
        why=Reason::stopped;
    if (why==Reason::none && (f.mono<t.mono || f.mono-t.mono>=dispatchBound ||
        absMs((f.wall-t.wall)-(f.mono-t.mono))>second)) why=Reason::clock_jump;
    if (why==Reason::none && at!=none && (slotOf(f.wall)!=at || f.wall-at>=dispatchBound)) why=Reason::missed_boundary;
    Ms latest=t.mono+dispatchBound;
    if (i && why==Reason::none) latest=std::min({latest,f.mono+i->expires-f.wall,handoffDeadline});
    Drive driven;
    if (why==Reason::none && !output_.open(length,latest,d0.stops,driven))
        why=Reason::interlock;
    Ledger confirmed=state_; reports.clear();
    if (why!=Reason::none) {
        confirmed.session={};
        disposition(confirmed,reports,f,why);
        save(std::move(confirmed),reports,true);
        return;
    }

    const auto actual=clock_.now();
    monoStart_=driven.onAt; deadline_=monoStart_+length; running_=true;
    confirmed.session.phase=Session::Phase::running;
    confirmed.session.accountedAt=stamp+(monoStart_-t.mono);
    confirmed.accountingFloor=confirmed.session.accountedAt;
    // f was the verified sample just before output; deadline/accounting never
    // use wall corrections observed after output.
    confirmed.session.started=f.wall+(monoStart_-f.mono);
    const Sample started{monoStart_,confirmed.session.started,f.quality};
    reports.push_back(fromSession(confirmed,Kind::session_started,started,confirmed.session));
    if (at!=none) reports.push_back(occurrence(confirmed,started,at,Outcome::started,Reason::none,
                                              request,confirmed.session.id));
    if (actual.mono<monoStart_) { breakStorage(); return; }
    save(std::move(confirmed),reports,true); // any error closes in breakStorage()
}

void Controller::noteDiscontinuity(Sample old, Sample now) {
    if (!old.usable() || !now.usable() || broken_ || state_.applied.ref.empty()) return;
    const auto floor=std::max(slotOf(old.wall),slotOf(now.wall));
    if (floor<=state_.scheduleFloor) return;
    Ledger d=state_; d.scheduleFloor=floor;
    const auto slot=slotOf(now.wall);
    if (slot>state_.scheduleFloor && due(d.applied.schedule,slot) &&
        journal_.queued()<backlogMax-(running_ ? 3 : 1)) {
        auto e=occurrence(d,now,slot,Outcome::skipped,Reason::clock_jump);
        save(std::move(d),{e});
    } else save(std::move(d),{});
}

void Controller::tick(bool localStart, bool busyEdge, const Exchange* x, const std::vector<EventId>& sent) {
    if (maintenance_) return;
    auto now=clock_.now();
    auto d=output_.read();
    Reason priority=Reason::none;
    const bool busyAtArrival=running_ || busyEdge;
    if (d.stops!=observedStops_) {
        observedStops_=d.stops;
        handleStop(now,Source::local_button,newId("st"));
        priority=Reason::stopped;
    }
    if (x) for (const auto& s : x->stops) {
        if (!s.target || s.request.empty()) continue;
        const bool deferred=std::any_of(state_.deferredStops.begin(),state_.deferredStops.end(),[&](const Event& e) {
            return e.boot==s.boot && e.request==s.request;
        });
        if (deferred || journal_.hasStopReceipt(s.boot,s.request)) continue;
        if (!journal_.healthy()) breakStorage();
        const bool stale=s.boot!=bootId_;
        if (stale) {
            const auto h=idFingerprint(s.request.view());
            if (has(staleStops_,h) || staleStops_.size()>=stopsPerBoot) continue;
            staleStops_.push_back(h);
        } else {
            if (std::find(stopSeen_.begin(),stopSeen_.end(),s.request)!=stopSeen_.end()) continue;
            if (stopSeen_.size()<stopsPerBoot) stopSeen_.push_back(s.request);
            else remoteStopCapacity_=true; // still CLOSE; no new OPEN if proof of dedup is exhausted
        }
        handleStop(clock_.now(),Source::remote,s.request,stale);
        if (!stale) priority=Reason::remote_stop;
    }
    if (!observe(now)) return;
    d=output_.read();
    if (running_ && (!d.on || now.mono>=deadline_ || !d.usable)) {
        const auto why=!d.usable ? Reason::unsafe_output :
            (d.offReason!=Reason::none ? d.offReason : (now.mono>=deadline_ ? Reason::deadline : Reason::unsafe_output));
        Ledger next=state_; std::vector<Event> reports;
        finish(next,reports,now,output_.close(why),why);
        save(std::move(next),reports,true);
    } else if (!running_ && d.on) {
        output_.close(Reason::unsafe_output); possible_=true; ++change_;
        if (!broken_) {
            Ledger next=state_; next.unknown=true;
            auto e=make(next,Kind::fault,now,Source::none,Reason::unsafe_output);
            save(std::move(next),{e},true);
        }
    }
    if (!broken_ && (!journal_.healthy() || (running_ && now.mono-lastProbe_>=second))) {
        lastProbe_=now.mono;
        if (!journal_.verifyTail()) breakStorage();
    }
    if (x && !broken_) {
        std::vector<EventId> acks;
        for (const auto& id : x->acked)
            if (std::find(sent.begin(),sent.end(),id)!=sent.end()) acks.push_back(id);
        if (!acks.empty() && ((!running_ && !journal_.prepare(2)) || !journal_.commit(nullptr,{},acks)))
            breakStorage();
    }
    flushStopReceipts(); // including space freed by ACK after immediate CLOSE
    if (state_.pending.receivedSequence && !broken_) {
        const auto why=remote(state_.pending.intent,clock_.now());
        if (why!=Reason::none || busyAtArrival) {
            Ledger next=state_; std::vector<Event> reports;
            cancelPending(next,reports,now,busyAtArrival ? Reason::busy : why);
            save(std::move(next),reports);
        }
    }
    if (x) apply(*x,clock_.now());

    const Sample old=prior_;
    prior_=now;
    const bool usable=old.usable() && now.usable();
    const bool discontinuity=usable && (now.mono-old.mono>dispatchBound ||
        absMs((now.wall-old.wall)-(now.mono-old.mono))>750);
    if (discontinuity) noteDiscontinuity(old,now);
    const auto slot=slotOf(now.wall);
    const bool crossed=usable && !discontinuity && slot>slotOf(old.wall) && now.wall-slot<dispatchBound;
    if (priority==Reason::none && busyAtArrival) priority=Reason::busy;
    if (localStart) local(now,priority);
    scheduled(now,crossed,priority);
    if (x) {
        for (const auto& g : x->grants) grant(g,clock_.now(),priority);
        for (const auto& c : x->candidates) candidate(c,clock_.now(),priority);
    }
}

Snapshot Controller::snapshot() {
    Snapshot s;
    const auto d=output_.read();
    s.time=clock_.now(); s.epoch=epoch_; s.applied=state_.applied.ref; s.on=d.on;
    if (d.on && running_ && state_.session.phase==Session::Phase::running) {
        s.active=state_.session;
        s.elapsedSeconds=uint32_t(std::max(Ms{0},s.time.mono-monoStart_)/second);
    }
    s.blocked=gate(s.time,d);
    s.ready=s.blocked==Reason::none;
    return s;
}
std::vector<Event> Controller::reports() {
    auto e=journal_.pending(state_.pending.receivedSequence);
    if (!journal_.healthy()) breakStorage();
    for (const auto& r: e) {
        if (r.sequence==state_.pending.receivedSequence) offeredSequence_=r.sequence;
    }
    return e;
}

bool Controller::enterMaintenance() {
    if (running_ || output_.read().on || maintenance_) return false;
    maintenance_=true;
    output_.close(Reason::maintenance);
    if (epoch_==UINT32_MAX) possible_=true; else ++epoch_;
    if (state_.pending.receivedSequence) {
        Ledger next=state_; std::vector<Event> reports;
        cancelPending(next,reports,clock_.now(),Reason::maintenance);
        save(std::move(next),reports);
    }
    ++change_;
    return true;
}
void Controller::leaveMaintenance() {
    prior_={}; maintenance_=false; ++change_;
}
}
