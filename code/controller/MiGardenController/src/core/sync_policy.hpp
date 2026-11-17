#pragma once
#include "types.hpp"
#include <algorithm>

namespace mg {
class SyncPolicy {
public:
    bool due(Ms now,bool link,uint64_t change) {
        if (!link) { online_=false; return false; }
        if (blocked_) return false;
        if (!online_) { online_=true; if (!serverDelay_) next_=now; }
        return now>=next_ || (!failed_ && !serverDelay_ && change!=sent_);
    }
    void sent(uint64_t c) { sent_=c; }
    void ok(Ms now,bool active,bool immediate,int retryAfter) {
        failed_=0;
        serverDelay_=retryAfter>0;
        next_=now+(serverDelay_ ? Ms(retryAfter)*second : immediate ? 0 : (active?5:15)*second);
    }
    void transient(Ms now,uint32_t random,int retryAfter=-1) {
        constexpr int delays[]={5,15,30,60};
        const auto base=delays[std::min(failed_,3U)]*second;
        if (failed_<4) ++failed_;
        serverDelay_=retryAfter>0;
        next_=now+std::max(base+random%1001,serverDelay_ ? Ms(retryAfter)*second : Ms{0});
    }
    void permanent() { blocked_=true; }
    void reset() { *this={}; }
    Ms next() const { return next_; }
    bool blocked() const { return blocked_; }
private:
    bool online_=false,blocked_=false,serverDelay_=false;
    unsigned failed_=0;
    Ms next_=0;
    uint64_t sent_=0;
};
}
