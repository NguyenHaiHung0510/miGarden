#pragma once
#include "../core/types.hpp"
#include <algorithm>

namespace mg {
// Pure elapsed-time policy. No scan, credential, storage or control access.
class WifiPolicy {
public:
    enum class Action : uint8_t { none, preferred, fallback, disconnect };
    enum class Phase : uint8_t { ready, trying, connected, backoff };
    static constexpr Ms attemptWindow=15*second,firstBackoff=5*second,maxBackoff=60*second;
    explicit WifiPolicy(bool fallback):fallback_(fallback){}
    Action step(Ms now,bool connected,bool linkLost=false) {
        if(now<0)return Action::none;
        if(phase_==Phase::connected) {
            if(connected&&!linkLost)return Action::none; // healthy link is sticky, even fallback
            backoff_=firstBackoff;
            return attempt(now,1);
        }
        if(phase_==Phase::ready)return attempt(now,1);
        if(phase_==Phase::trying) {
            if(connected) {
                phase_=Phase::connected;backoff_=firstBackoff;
                return Action::none;
            }
            if(now<next_)return Action::none;
            if(slot_==1&&fallback_)return attempt(now,2);
            phase_=Phase::backoff;slot_=0;next_=now+backoff_;
            backoff_=std::min(2*backoff_,maxBackoff);
            return Action::disconnect; // cancel late association before sleeping
        }
        if(now>=next_)return attempt(now,1);
        return Action::none;
    }
    unsigned slot()const{return slot_;}
    Phase phase()const{return phase_;}
    static const char* label(Phase p) {
        constexpr const char* labels[]={"READY","TRYING","CONNECTED","BACKOFF"};
        return labels[unsigned(p)];
    }
private:
    bool fallback_;
    Phase phase_=Phase::ready;
    unsigned slot_=0;
    Ms next_=0,backoff_=firstBackoff;
    Action attempt(Ms now,unsigned slot) {
        phase_=Phase::trying;slot_=slot;next_=now+attemptWindow;
        return slot==1?Action::preferred:Action::fallback;
    }
};
}
