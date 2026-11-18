#pragma once
#include "../core/types.hpp"
#include <ArduinoJson.h>

namespace mg {
struct WifiProfile {
    Text<32> ssid, fallbackSsid;
    Text<64> password, fallbackPassword;
    bool hasFallback=false;
};
inline bool wifiAscii(std::string_view s) {
    for(unsigned char c:s)if(c<32||c>126)return false;
    return true;
}
inline bool wifiPassword(std::string_view s) {
    if(!wifiAscii(s))return false;
    if(s.empty())return true;
    if(s.size()<8||s.size()>64)return false;
    if(s.size()==64)for(char c:s)
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
    return true;
}
template<size_t N>inline bool wifiText(JsonVariantConst v,Text<N>& out) {
    if(!v.is<JsonString>())return false;
    const auto s=v.as<JsonString>();
    return out.set({s.c_str(),s.size()});
}
// This exact parser is shared by normal firmware, commissioner and host tests.
inline bool decodeWifiProfile(JsonObjectConst o,WifiProfile& out) {
    WifiProfile next;
    if(o.isNull()||!wifiText(o["wifi_ssid"],next.ssid)||next.ssid.empty()||!wifiAscii(next.ssid.view())||
       !wifiText(o["wifi_password"],next.password)||!wifiPassword(next.password.view()))return false;
    bool ssidPresent=false,passwordPresent=false;
    for(auto pair:o) {
        const std::string_view key=pair.key().c_str();
        if(key=="wifi_fallback_ssid")ssidPresent=true;
        if(key=="wifi_fallback_password")passwordPresent=true;
    }
    if(ssidPresent!=passwordPresent)return false;
    if(ssidPresent) {
        if(!wifiText(o["wifi_fallback_ssid"],next.fallbackSsid)||next.fallbackSsid.empty()||
           !wifiAscii(next.fallbackSsid.view())||next.ssid==next.fallbackSsid||
           !wifiText(o["wifi_fallback_password"],next.fallbackPassword)||
           !wifiPassword(next.fallbackPassword.view()))return false;
        next.hasFallback=true;
    }
    out=next;
    return true;
}
}
