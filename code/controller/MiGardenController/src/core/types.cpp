#include "types.hpp"
#include <algorithm>
#include <cstdio>

namespace mg {
bool valid(const Config& c) {
    if (!visible(c.ref.view())) return false;
    const auto& p = c.policy;
    const auto& s = c.schedule;
    if (!durationValid(p.manual) || !durationValid(p.session) || !durationValid(p.cumulative) ||
        p.window != windowSeconds || p.manual > p.session || p.session > p.cumulative ||
        p.cumulative > p.window || !durationValid(s.duration) || s.duration > p.session ||
        s.from < minUtc || s.from > maxUtc || (s.until != none && (s.until <= s.from || s.until > maxUtc)))
        return false;
    const bool atMinute = s.at >= minUtc && s.at <= maxUtc && s.at % minute == 0;
    switch (s.type) {
        case Recurrence::once:
            return atMinute && s.weekdays == 0 && s.intervalMinutes == 0 && s.localMinute == 0;
        case Recurrence::daily:
            return s.localMinute < 1440 && s.weekdays == 0 && s.at == none && s.intervalMinutes == 0;
        case Recurrence::weekly:
            return s.localMinute < 1440 && s.weekdays > 0 && s.weekdays < 128 &&
                s.at == none && s.intervalMinutes == 0;
        case Recurrence::interval:
            return atMinute && s.intervalMinutes > 0 && s.localMinute == 0 && s.weekdays == 0;
    }
    return false;
}
Ms slotOf(Ms wall) { return wall < 0 ? none : wall / minute * minute; }
bool due(const Schedule& s, Ms slot) {
    if (!s.enabled || slot < s.from || slot < 0 || slot % minute ||
        (s.until != none && slot >= s.until)) return false;
    if (s.type == Recurrence::once) return slot == s.at;
    if (s.type == Recurrence::interval)
        return s.intervalMinutes && slot >= s.at && (slot - s.at) % (Ms(s.intervalMinutes) * minute) == 0;
    const Ms local = slot + 7 * 60 * minute;
    const int weekday = int((local / day + 3) % 7); // 1970-01-01 was Thursday
    return (s.type == Recurrence::daily || (s.type == Recurrence::weekly && (s.weekdays & (1U << weekday)))) &&
        local % day / minute == s.localMinute;
}
Ms rollingUsage(const std::vector<Range>& intervals, Ms end) {
    const Ms begin = end - windowSeconds * second;
    Ms total = 0;
    for (const auto& i : intervals) {
        if (i.start >= end) break;
        total += std::max(Ms{0}, std::min(i.end, end) - std::max(i.start, begin));
    }
    return total;
}
Ms rollingPeak(const std::vector<Range>& intervals, Range n) {
    // From n.start to n.end the new interval enters at rate 1 and at most
    // one previously recorded interval leaves at rate 1 (disjoint ledger,
    // single output; n is shorter than/equal to W). Peak is at n.end.
    // After it ends no more output enters. Each term is an exact overlap.
    return rollingUsage(intervals, n.end) + n.end - n.start;
}
uint64_t idFingerprint(std::string_view s) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h; // collision refuses a new ID; never confers authority
}
EventId eventId(const Event& e) {
    char buf[97];
    std::snprintf(buf, sizeof buf, "%s.%llu", e.boot.c_str(), static_cast<unsigned long long>(e.sequence));
    EventId id; id.set(buf); return id;
}
Id occurrenceId(Ms slot) {
    char b[48];
    std::snprintf(b, sizeof b, "zone-a.%lld", static_cast<long long>(slot / minute));
    Id id; id.set(b); return id;
}
const char* label(Quality x) {
    constexpr const char* v[] = {"unknown","synced","holdover"}; return v[unsigned(x)];
}
const char* label(Source x) {
    constexpr const char* v[] = {"","schedule","local_button","remote"}; return v[unsigned(x)];
}
const char* label(Kind x) {
    constexpr const char* v[] = {"config_applied","config_rejected","request_received","request_rejected",
        "session_started","session_ended","fault","recovered","stop_processed","occurrence_decided"};
    return v[unsigned(x)];
}
const char* label(Reason x) {
    constexpr const char* v[] = {"","BUSY","STOP_HELD","LOCAL_STOP","REMOTE_STOP","TIME_UNKNOWN","CLOCK_JUMP",
        "CONFIG_MISSING","INVALID_CONFIG","CONFIG_CONFLICT","STALE_CONFIG","CONFIG_CAPACITY","WRONG_TARGET",
        "CONTEXT_CHANGED","STALE_BOOT","STALE_EPOCH","INVALID_DURATION","EXPIRED","INVALID_TTL","NO_CANDIDATE",
        "INTENT_CONFLICT","ROLLING_LIMIT","INTERVAL_CAPACITY","REPORT_CAPACITY","STORAGE_FAULT","OUTCOME_UNKNOWN",
        "OUTPUT_FAULT","REBOOT_INTERRUPTED","INTERLOCK","MISSED_BOUNDARY","DEADLINE","REMOTE_CAPACITY",
        "MAINTENANCE","UNSAFE_OUTPUT"};
    return v[unsigned(x)];
}
const char* label(Outcome x) {
    constexpr const char* v[] = {"","completed","stopped","fault","unknown","close_requested","already_off",
        "unconfirmed","started","skipped","rejected"};
    return v[unsigned(x)];
}
namespace {
bool leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
int monthDays(int y, int m) {
    constexpr int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return days[m-1] + (m == 2 && leap(y));
}
int daysBeforeYear(int y) {
    const int n = y - 1;
    return n * 365 + n / 4 - n / 100 + n / 400;
}
bool number(std::string_view s, size_t at, size_t len, int& n) {
    if (at + len > s.size()) return false;
    n = 0;
    for (size_t i = at; i < at + len; ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        n = n * 10 + s[i] - '0';
    }
    return true;
}
}
bool parseUtc(std::string_view s, Ms& out) {
    int y,m,d,h,n,ss;
    if (s.size() < 20 || s[4]!='-' || s[7]!='-' || s[10]!='T' || s[13]!=':' || s[16]!=':' ||
        !number(s,0,4,y) || !number(s,5,2,m) || !number(s,8,2,d) || !number(s,11,2,h) ||
        !number(s,14,2,n) || !number(s,17,2,ss) || y < 1970 || m < 1 || m > 12 ||
        d < 1 || d > monthDays(y,m) || h > 23 || n > 59 || ss > 59) return false;
    size_t at = 19;
    int fraction = 0;
    if (s[at] == '.') {
        const size_t begin = ++at;
        while (at < s.size() && s[at] >= '0' && s[at] <= '9') {
            if (at - begin < 3) fraction = fraction * 10 + s[at] - '0';
            ++at;
        }
        if (at == begin || at - begin > 9) return false;
        for (size_t i = at - begin; i < 3; ++i) fraction *= 10;
    }
    const auto suffix = s.substr(at);
    if (suffix != "Z" && suffix != "+00:00") return false;
    int days = daysBeforeYear(y) - daysBeforeYear(1970) + d - 1;
    for (int i = 1; i < m; ++i) days += monthDays(y,i);
    out = days * day + h * 60 * minute + n * minute + ss * second + fraction;
    return true;
}
void formatUtc(Ms t, char (&out)[32]) {
    if (t < minUtc || t > maxUtc) { out[0] = 0; return; }
    const int days = int(t / day);
    int lo = 1970, hi = 10000;
    while (lo + 1 < hi) {
        const int mid = (lo + hi) / 2;
        if (daysBeforeYear(mid) - daysBeforeYear(1970) <= days) lo = mid; else hi = mid;
    }
    int m = 1, d = days - daysBeforeYear(lo) + daysBeforeYear(1970);
    while (m < 12 && d >= monthDays(lo,m)) d -= monthDays(lo,m++);
    const int tod = int(t % day);
    char b[64];
    std::snprintf(b, sizeof b, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
        lo, m, d+1, tod/3600000, tod/60000%60, tod/1000%60, tod%1000);
    std::memcpy(out,b,25); // fixed ISO layout, including NUL
}
}
