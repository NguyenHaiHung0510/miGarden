#include "../MiGardenController/src/commissioning/transaction.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace mg;
using namespace mg::commission;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(false)
Digest d(char c){Digest v;v.set(std::string(64,c));return v;}
struct Fake:Port{
    Observation o;
    bool lab=false,failFormat=false,failJournal=false,failNamespace=false,damageOthers=false;
    unsigned formats=0,journals=0,nvsWrites=0;
    std::string existingLab="SECRET-LAB",other="KEEP",secret;
    Fake(){o.layout=o.namespaceAbsent=o.journalAbsent=o.storageReadable=true;o.fs=Fs::blank;
        o.partition=d('1');o.nvs=d('2');o.data=d('3');o.otherNvs=d('4');o.otherFiles=d('5');}
    void target(bool x)override{lab=x;}
    bool observe(Observation& out)override{out=o;return true;}
    bool formatBlank()override{++formats;if(failFormat){o.fs=Fs::unknown;return false;}o.fs=Fs::verified;return true;}
    bool createJournal()override{++journals;o.journalAbsent=false;if(failJournal)return false;o.journal=d('7');return true;}
    bool createNamespace(std::string_view value)override{
        CHECK(!lab);++nvsWrites;o.namespaceAbsent=false;
        if(failNamespace)return false;
        secret=value;o.profile=d('6');
        if(damageOthers)o.otherNvs=d('8');
        return true;
    }
    bool verifyCreated(Observation& out)override{return observe(out);}
};
Plan make(){Plan p;p.transaction.set("test-transaction");p.partition=d('1');p.nvs=d('2');p.data=d('3');
    p.otherNvs=d('4');p.otherFiles=d('5');p.profile=d('6');p.journal=d('7');p.targetHash=d('9');
    p.targetBuild.set("pinned");p.targetBytes=100;p.approved=p.backupConfirmed=p.allowBlankInit=true;return p;}
void releasePreserves(){
    Fake f;Transaction t(f);const auto p=make();
    Observation post;
    CHECK(t.apply(p,"SECRET-RELEASE",&post)==Result::ok&&post.profile==p.profile&&post.journal==p.journal);
    CHECK(f.existingLab=="SECRET-LAB"&&f.other=="KEEP"&&f.secret=="SECRET-RELEASE");
    CHECK(f.formats==1&&f.journals==1&&f.nvsWrites==1);
    CHECK(t.apply(p,"SECRET-RELEASE")==Result::transaction_used);
    CHECK(t.verify(p)==Result::ok);
}
void labNoNvsMutation(){
    Fake f;auto p=make();p.lab=true;f.o.namespaceAbsent=false;f.o.profile=p.profile;
    Transaction t(f);CHECK(t.apply(p,"SECRET-LAB")==Result::ok);
    CHECK(f.nvsWrites==0&&f.o.nvs==p.nvs&&f.existingLab=="SECRET-LAB");
}
void conflictsNeverOverwrite(){
    for(bool file:{true,false}){
        Fake f;auto p=make();if(file)f.o.journalAbsent=false;else f.o.namespaceAbsent=false;
        Transaction t(f);CHECK(t.apply(p,"secret")==Result::target_exists);
        CHECK(f.formats+f.journals+f.nvsWrites==0);
    }
}
void partialWriteStaysHold(){
    for(unsigned kind=0;kind<3;++kind){
        Fake f;auto p=make();
        f.failFormat=kind==0;f.failJournal=kind==1;f.failNamespace=kind==2;
        Transaction t(f);CHECK(t.apply(p,"secret")!=Result::ok);
        CHECK(t.used()&&t.apply(p,"secret")==Result::transaction_used);
        Transaction afterBoot(f);
        CHECK(afterBoot.apply(p,"secret")!=Result::ok);
        CHECK(f.existingLab=="SECRET-LAB"&&f.other=="KEEP");
    }
}
void wrongPreimageApprovalLayout(){
    for(int n=0;n<5;++n){
        Fake f;auto p=make();
        if(n==0)p.approved=false;
        if(n==1)p.backupConfirmed=false;
        if(n==2)p.allowBlankInit=false;
        if(n==3)f.o.layout=false;
        if(n==4)f.o.data=d('0');
        Transaction t(f);CHECK(t.apply(p,"secret")!=Result::ok);
        CHECK(f.formats+f.journals+f.nvsWrites==0);
    }
}
void verifiedFilesystemNoFormat(){
    Fake f;f.o.fs=Fs::verified;auto p=make();p.allowBlankInit=false;
    Transaction t(f);CHECK(t.apply(p,"secret")==Result::ok&&f.formats==0);
}
void preservationMismatch(){
    Fake f;f.damageOthers=true;Transaction t(f);
    CHECK(t.apply(make(),"secret")==Result::preserve_fault);
}
int main(){
    unsigned passed=0,failed=0;
    auto test=[&](const char* name,auto fn){
        try{fn();++passed;std::cout<<"PASS "<<name<<"\n";}
        catch(const std::exception&){++failed;std::cerr<<"FAIL "<<name<<"\n";}
    };
    test("releasePreserves",releasePreserves);test("labNoNvsMutation",labNoNvsMutation);
    test("conflictsNeverOverwrite",conflictsNeverOverwrite);test("partialWriteStaysHold",partialWriteStaysHold);
    test("wrongPreimageApprovalLayout",wrongPreimageApprovalLayout);test("verifiedFilesystemNoFormat",verifiedFilesystemNoFormat);
    test("preservationMismatch",preservationMismatch);
    std::cout<<"TOTAL "<<passed<<" PASS, "<<failed<<" FAIL\n";return failed!=0;
}
