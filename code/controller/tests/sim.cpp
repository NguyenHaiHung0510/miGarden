#include "fixture.hpp"
#include "wire.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace mg;
using namespace mg::test;

namespace {
void usage(std::ostream& out) {
    out<<"usage: sim [--device-id ID] | sim genesis <new-offline-file>\n"
        "       sim recovery-check <private-journal-copy>\n"
        "stdin: snapshot, exchange <v1 response-json>, advance <ms>, gap <ms>, jump <UTC>,\n"
        "clock unknown|synced|holdover, start, stop, release, reboot, diag, quit\n";
}
}

int main(int argc,char** argv) {
    if (argc==2 && (std::string_view(argv[1])=="--help" || std::string_view(argv[1])=="-h")) {
        usage(std::cout); return 0;
    }
    if (argc==2 && std::string_view(argv[1])=="--version") { std::cout<<protocol<<" host simulator\n"; return 0; }
    if (argc==3 && std::string_view(argv[1])=="genesis") {
        const auto image=Journal::genesis(); // blank ledger, no config, keys, history or synthetic events
        std::ifstream existing(argv[2],std::ios::binary);
        if (existing.good()) { std::cerr<<"File already exists; genesis writes only a new file.\n"; return 2; }
        std::ofstream f(argv[2],std::ios::binary);
        f.write(reinterpret_cast<const char*>(image.data()),image.size());
        return f.good()?0:2;
    }
    if (argc==3 && (std::string_view(argv[1])=="verify-journal"||std::string_view(argv[1])=="recovery-check")) {
        std::ifstream input(argv[2],std::ios::binary|std::ios::ate);
        if (!input) return 2;
        const auto size=input.tellg();
        if(size<=0||size>Journal::maxLog)return 2;
        input.seekg(0);
        Memory memory;
        memory.live.assign(std::istreambuf_iterator<char>(input),{});
        Journal journal(memory);
        Ledger state;
        const bool valid=journal.restore(state);
        if(std::string_view(argv[1])=="recovery-check") {
            const bool eligible=valid&&(state.recoveryCause==RecoveryCause::reboot_running||
                                        state.recoveryCause==RecoveryCause::reboot_armed);
            std::cout<<"{\"valid\":"<<(valid?"true":"false")<<",\"eligible_reboot\":"<<(eligible?"true":"false")
                <<",\"legacy_proof\":"<<(journal.inferredReboot()?"true":"false")
                <<",\"operator_proof_required\":"<<(valid&&state.unknown&&!eligible?"true":"false")
                <<",\"conservative_charge_ms\":"<<state.recoveryDebt<<"}\n";
            return valid?0:2;
        }
        std::cout<<"{\"valid\":"<<(valid?"true":"false");
        if (valid) std::cout<<",\"queued\":"<<journal.queued()<<",\"unknown\":"<<(state.unknown?"true":"false")
            <<",\"intervals\":"<<state.history.size()<<",\"config_refs\":"<<state.configRefs.size()
            <<",\"deferred_stops\":"<<state.deferredStops.size();
        std::cout<<"}\n";
        return valid?0:2;
    }
    Id device=id("qa-controller-a");
    if (argc>1 && (argc!=3 || std::string_view(argv[1])!="--device-id" ||
                   !visible(argv[2]) || !device.set(argv[2]))) {
        usage(std::cerr); return 2;
    }
    std::cout<<std::unitbuf; // parent can read one response after each stdin line
    Fixture f;
    std::string line;
    while (std::getline(std::cin,line)) {
        std::istringstream cmd(line);
        std::string word,arg;
        cmd>>word;
        bool validCommand=true, showExchange=false;
        if (word=="snapshot") showExchange=true;
        else if (word=="exchange") {
            const auto pos=line.find(' ');
            Exchange x;
            if (pos==line.npos || decodeExchange(std::string_view(line).substr(pos+1),x)!=WireError::none) validCommand=false;
            else { f.exchange(x); showExchange=true; }
        } else if (word=="advance" || word=="gap") {
            Ms n;
            if (!(cmd>>n) || n<0 || n>day) validCommand=false;
            else if (word=="advance") f.advance(n);
            else f.gap(n);
        } else if (word=="jump") {
            Ms t;
            if (!(cmd>>arg) || !parseUtc(arg,t)) validCommand=false;
            else { ++f.clock.s.mono; f.clock.s.wall=t; f.tick(); }
        } else if (word=="clock") {
            cmd>>arg;
            if (arg=="unknown") f.clock.s.quality=Quality::unknown;
            else if (arg=="synced") f.clock.s.quality=Quality::synced;
            else if (arg=="holdover") f.clock.s.quality=Quality::holdover;
            else validCommand=false;
            f.tick();
        } else if (word=="start") f.tick(true);
        else if (word=="stop") { f.actuator.pressStop(); f.tick(); }
        else if (word=="release") { f.actuator.releaseStop(); f.tick(); }
        else if (word=="reboot") f.reboot();
        else if (word=="quit") return 0;
        else if (word!="diag") validCommand=false;
        if (!validCommand) std::cout<<"{\"error\":\"command_or_wire\"}\n";
        else if (showExchange) {
            const auto events=f.offer();
            std::string out;
            if (encodeExchange(device,f.c->bootId(),f.c->snapshot(),events,out))
                std::cout<<out<<"\n";
            else std::cout<<"{\"error\":\"encode\"}\n";
        } else {
            const auto s=f.c->snapshot();
            std::cout<<"{\"snapshot\":"<<encodeSnapshot(s)<<",\"control_epoch\":"<<s.epoch
                <<",\"opens\":"<<f.actuator.opens<<",\"queued\":"<<f.c->queued()
                <<",\"journal_bytes\":"<<f.c->logBytes()<<",\"evidence_gaps\":"<<f.c->evidenceGaps()<<"}\n";
        }
    }
}
