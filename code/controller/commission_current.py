"""Prepare/validate a preserving commissioner plan; explicit owner-run capture/execute, no auto-discovery."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from uuid import uuid4

import commission as common
from commission import Stop, need, digest, load_unique, read_private, bind_profile
import private_acl

ROOT=common.ROOT
LAYOUT="148b959cbff1c38aa8e1d5c0ba9d612c54997b945e56a63f41223eef650653a1"
REQUIRED={"profile.json","device-binding.json","capture.json","backup-nvs.bin","backup-data.bin",
          "target-app-manifest.json","commissioner-manifest.json","request.json","OWNER-SEQUENCE.txt","genesis.journal",
          "owner-approval.template.json","finalize-approval.template.json"}

def put(path:Path,value):
    data=value if isinstance(value,bytes) else (json.dumps(value,indent=2)+"\n").encode()
    private_acl.write_new(path,data)

def app(path:Path,profile:str):
    m=load_unique(path.read_bytes())
    need(m["profile"]==profile and m["artifact_gate"]=="PASS" and m["compile_exit"]==0 and
         m["wire"]=="migarden.zone-a.v1" and m["configured_flash_bytes"]==0x400000 and
         bool(re.fullmatch(r"n01_"+profile+r"_[0-9]{8}T[0-9]{6}_[a-f0-9]{12}",m["build_id"])) and
         m["app0"]=={"start":0x10000,"bytes":0x140000} and m["app1"]=={"start":0x150000,"bytes":0x140000} and
         m["data"]["start"]==0x290000 and m["data"]["bytes"]==0x160000,"ARTIFACT_PROFILE_OR_LAYOUT")
    def checked(suffix):
        rel=f'.local/n01/build/{m["build_id"]}/MiGardenController.ino.{suffix}'
        candidates=[f for f in m["artifacts"] if f["path"]==rel]
        need(len(candidates)==1,"ARTIFACT_FILE_MISSING")
        e=candidates[0];b=(ROOT/rel).read_bytes()
        need(len(b)==e["bytes"] and digest(b)==e["sha256"],"ARTIFACT_DIGEST")
        return e,b
    e,b=checked("bin")
    partition,_=checked("partitions.bin")
    need(partition["bytes"]==3072 and partition["sha256"]==LAYOUT and
         32<=len(b)<=0x140000 and b[0]==0xe9 and m["build_id"].encode("ascii") in b,
         "ARTIFACT_IMAGE_OR_PARTITION")
    if profile=="release":
        need(m.get("gpio_outputs_in_profile") is True,"RELEASE_PROFILE_REQUIRED")
    if profile=="commissioner":
        need(m.get("gpio_outputs_in_profile") is False,"COMMISSIONER_GPIO_MUST_BE_INACTIVE")
        need(m.get("commissioner_injection") is True and m["usb_compatible"] is True and
             b"MG-COMMISSION-V1" in b and b"MG-LAB-USB-PROTOCOL-1-N01" in b,"COMMISSIONER_REQUIRED")
    else:
        need(b"MG-COMMISSION-V1" not in b,"NORMAL_TARGET_REQUIRED")
    return m, e

def nvs_profile(path:Path,profile:str,parser:Path):
    raw=read_private(path)
    need(len(raw)==0x5000,"NVS_BACKUP_SIZE")
    p=common.parser_at(parser).NVS_Partition("private-backup",bytearray(raw))
    entries=[]
    for page in p.pages:
        if page.header["status"]!="Empty":
            need(page.header["status"] in ("Active","Full") and
                 page.header["crc"]["original"]==page.header["crc"]["computed"],"NVS_BACKUP_CRC")
        for e in page.entries:
            if e.state=="Written":
                need(e.metadata["crc"]["original"]==e.metadata["crc"]["computed"],"NVS_BACKUP_CRC")
                if e.metadata["type"] in ("blob_data","string"):
                    need(e.metadata["crc"]["data_original"]==e.metadata["crc"]["data_computed"],"NVS_BACKUP_CRC")
                entries.append(e)
    namespace=common.PROFILES[profile][0]
    if profile=="release":
        need(any(e.metadata["namespace"]==0 and e.key=="mg-lab-v1" for e in entries),
             "EXISTING_LAB_NAMESPACE_REQUIRED",3)
    namespaces=[e for e in entries if e.metadata["namespace"]==0 and e.key==namespace]
    if not namespaces:return None
    need(len(namespaces)==1,"NVS_NAMESPACE_CONFLICT")
    if profile=="release":return b"" # any namespace, even empty, is a conflict
    i=namespaces[0].data["value"]
    values=[e for e in entries if e.metadata["namespace"]==i and e.key=="profile-v1"]
    chunks=sorted([e for e in values if e.metadata["type"]=="blob_data"],key=lambda e:e.metadata["chunk_index"])
    index=[e for e in values if e.metadata["type"]=="blob_index"]
    need(len(index)==1 and len(values)==len(chunks)+1 and index[0].data["chunk_count"]==len(chunks),
         "NVS_NAMESPACE_INCOMPLETE")
    b=b"".join(b"".join(bytes(x.raw) for x in e.children)[:e.data["size"]] for e in chunks)
    need(len(b)==index[0].data["size"],"NVS_NAMESPACE_INCOMPLETE")
    return b

def capture_valid(folder:Path,profile:str):
    # The containing capture directory can also be a sealed bundle; its ACLs
    # are checked separately by validate. Verify just the three capture inputs.
    private_acl.verify(folder)
    record=load_unique(read_private(folder/"capture.json"))
    need(record["profile"]==profile and record["complete"] is True,"CAPTURE_INCOMPLETE")
    o=record["observation"]
    need(o["layout_verified"] is True and o["partition_sha256"]==LAYOUT and o["journal_absent"] is True,
         "LAYOUT_OR_EXISTING_JOURNAL")
    for name,size,key in (("backup-nvs.bin",0x5000,"nvs_sha256"),("backup-data.bin",0x160000,"data_sha256")):
        b=read_private(folder/name)
        need(len(b)==size and digest(b)==o[key],"CAPTURE_HASH")
    if o["filesystem"]=="blank":
        need(read_private(folder/"backup-data.bin")==b"\xff"*0x160000,"BLANK_GUARD")
    else:need(o["filesystem"]=="verified","UNVERIFIED_FILESYSTEM")
    for key in ("other_nvs_sha256","other_files_sha256"):
        need(bool(re.fullmatch("[a-f0-9]{64}",o[key])),"CAPTURE_INVENTORY")
    return record

def prepare(a):
    capture=common.private(a.capture)
    record=capture_valid(capture,a.profile)
    need(digest(read_private(capture/"capture.json"))==a.capture_sha256,"CAPTURE_PIN_REQUIRED")
    private_input=sys.stdin.buffer.read(8193) if a.private_config=="-" else read_private(Path(a.private_config))
    profile=common.profile_bytes(private_input,a.profile)
    binding=read_private(a.device_binding)
    bind_profile(profile,binding,a.profile)
    existing=nvs_profile(capture/"backup-nvs.bin",a.profile,a.nvs_parser)
    o=record["observation"]
    if a.profile=="lab":
        need(existing is not None and o["namespace_absent"] is False,"EXISTING_LAB_PROFILE_REQUIRED")
        need(load_unique(existing)==load_unique(profile) and digest(existing)==o["profile_sha256"],"LAB_PROFILE_MISMATCH")
        profile=existing # exact stored bytes; no rewrite, no formatting/canonicalization of existing NVS
    else:
        need(existing is None and o["namespace_absent"] is True,"DESTINATION_NAMESPACE_EXISTS",3)
    target, image=app(a.app_manifest,a.profile)
    targetBody=(ROOT/image["path"]).read_bytes()
    need(load_unique(profile)["device_token"].encode() not in targetBody,"CREDENTIAL_IN_APPLICATION")
    commissioner, commissionerImage=app(a.commissioner_manifest,"commissioner")
    need(load_unique(profile)["device_token"].encode() not in (ROOT/commissionerImage["path"]).read_bytes(),
         "CREDENTIAL_IN_APPLICATION")
    need(target["head_at_compile"]==commissioner["head_at_compile"],"ARTIFACT_SOURCE_MISMATCH")
    need(record["commissioner_build_id"]==commissioner["build_id"],"CAPTURE_COMMISSIONER_BUILD")
    need(digest(a.sim.read_bytes())==a.sim_sha256,"HOST_SIMULATOR_PIN")
    journal=read_private(a.genesis)
    check=subprocess.run([str(a.sim.resolve()),"verify-journal",str(a.genesis.resolve())],capture_output=True,timeout=15)
    need(check.returncode==0 and load_unique(check.stdout)==
         {"valid":True,"queued":0,"unknown":False,"intervals":0,"config_refs":0,"deferred_stops":0},
         "GENESIS_INVALID")
    out=common.private(a.out)
    need(not out.exists(),"DESTINATION_EXISTS",3)
    plan={"transaction_id":uuid4().hex,"profile":a.profile,"partition_sha256":LAYOUT,
          "nvs_sha256":o["nvs_sha256"],"data_sha256":o["data_sha256"],
          "other_nvs_sha256":o["other_nvs_sha256"],"other_files_sha256":o["other_files_sha256"],
          "profile_sha256":digest(profile),"journal_sha256":digest(journal),
          "target_sha256":image["sha256"],"target_build_id":target["build_id"],"target_bytes":image["bytes"],
          "backup_confirmed":True,"allow_blank_init":o["filesystem"]=="blank","approved":False}
    request={"action":"apply","plan":plan,"profile_json":profile.decode("utf-8")}
    if a.dry_run:
        print(json.dumps({"result":"DRY_RUN","profile":a.profile,"data_mode":o["filesystem"],
                          "writes_nvs":a.profile=="release","execution_granted":False}))
        return
    private_acl.directory(out)
    put(out/"profile.json",profile);put(out/"device-binding.json",binding)
    put(out/"capture.json",record)
    for n in ("backup-nvs.bin","backup-data.bin"):put(out/n,read_private(capture/n))
    put(out/"target-app-manifest.json",a.app_manifest.read_bytes())
    put(out/"commissioner-manifest.json",a.commissioner_manifest.read_bytes())
    put(out/"request.json",request)
    put(out/"genesis.journal",journal)
    put(out/"owner-approval.template.json",{"transaction_id":plan["transaction_id"],
        "request_sha256":digest((out/"request.json").read_bytes()),"allow_blank_init":plan["allow_blank_init"],"approve":False})
    put(out/"finalize-approval.template.json",{"transaction_id":plan["transaction_id"],"action":"flash-target-app",
                                              "target_sha256":image["sha256"],"approve":False})
    put(out/"OWNER-SEQUENCE.txt",(
        "REVIEW ONLY: no execution grant from this bundle.\n"
        "1. Verify separately reviewed commissioner manifest and exact digest, lease COM, use existing parent USB update transaction.\n"
        "2. Boot commissioner: GPIO control absent, network/control executor never starts; capture backups/status privately.\n"
        "3. This plan is prepared from that capture. Recheck capture/app binding/profile before Owner approval.\n"
        "4. Owner grants exact transaction ID/request digest/data scope and, for blank data, explicit filesystem initialization.\n"
        "5. Execute via this host tool with approval file. Snapshot must still match preimage; full FF is checked again before format.\n"
        "6. Transaction never overwrites a namespace/journal. Partial failure stays HOLD; do not erase/repack/reset to retry.\n"
        "7. Read-only verify returns actual postimage hashes and target profile/journal hashes, with other logical content unchanged.\n"
        "8. Explicitly update back to the separately pinned target app; no automatic switch or watering from commissioner.\n"
        "LAB: preserve existing NVS byte-for-byte, create only lab-v1.journal.\n"
        "Release: use NVS API only to create NEW mg-rel-v1/profile-v1; preserve every other key/value and create release-v1.journal.\n"
        "Device provisioning/data init is HOLD until Owner grants execution. Tests of this package were synthetic only.\n").encode())
    files={p.name:{"bytes":p.stat().st_size,"sha256":digest(p.read_bytes())} for p in out.iterdir() if p.is_file()}
    manifest={"version":"migarden.commission.current.v1","profile":a.profile,"files":files,
              "commissioner_build_id":commissioner["build_id"],"target_build_id":target["build_id"],
              "request_sha256":files["request.json"]["sha256"],"execution_granted":False,
              "simulator":{"path":str(a.sim.resolve()),"sha256":a.sim_sha256}}
    put(out/"manifest.json",manifest)
    private_acl.verify_tree(out)
    print(json.dumps({"result":"PREPARED","profile":a.profile,"manifest_sha256":digest((out/"manifest.json").read_bytes()),
                      "request_sha256":manifest["request_sha256"],"execution_granted":False}))

def validate(a):
    out=common.private(a.bundle);private_acl.verify_tree(out)
    raw=read_private(out/"manifest.json")
    need(digest(raw)==a.manifest_sha256,"REVIEW_MANIFEST_DIGEST")
    m=load_unique(raw)
    need(m["version"]=="migarden.commission.current.v1" and m["execution_granted"] is False and
         set(m["files"])==REQUIRED,"REQUIRED_BUNDLE_FILES")
    entries=list(out.iterdir())
    need(all(p.is_file() for p in entries),"EXTRA_OR_MISSING_BUNDLE_FILES")
    actual={p.name for p in entries}
    need(REQUIRED|{"manifest.json"}<=actual and
         actual<=REQUIRED|{"manifest.json","execution-attempt.json","finalize-attempt.json"},
         "EXTRA_OR_MISSING_BUNDLE_FILES")
    for name,e in m["files"].items():
        b=read_private(out/name)
        need(len(b)==e["bytes"] and digest(b)==e["sha256"],"BUNDLE_HASH")
    request=load_unique(read_private(out/"request.json"))
    need(set(request)=={"action","plan","profile_json"} and request["action"]=="apply" and
         request["plan"]["approved"] is False and m["request_sha256"]==digest(read_private(out/"request.json")),
         "REQUEST_BINDING")
    common.profile_bytes(read_private(out/"profile.json"),m["profile"])
    bind_profile(read_private(out/"profile.json"),read_private(out/"device-binding.json"),m["profile"])
    need(request["profile_json"].encode()==read_private(out/"profile.json"),"PROFILE_REQUEST_MISMATCH")
    need(request["plan"]["profile_sha256"]==digest(read_private(out/"profile.json")) and
         request["plan"]["journal_sha256"]==digest(read_private(out/"genesis.journal")),"REQUEST_CONTENT_DIGEST")
    sim=Path(m["simulator"]["path"])
    need(digest(sim.read_bytes())==m["simulator"]["sha256"],"HOST_SIMULATOR_PIN")
    parsed=subprocess.run([str(sim),"verify-journal",str(out/"genesis.journal")],capture_output=True,timeout=15)
    need(parsed.returncode==0 and load_unique(parsed.stdout)==
         {"valid":True,"queued":0,"unknown":False,"intervals":0,"config_refs":0,"deferred_stops":0},"GENESIS_INVALID")
    target,image=app(out/"target-app-manifest.json",m["profile"])
    commissioner,_=app(out/"commissioner-manifest.json","commissioner")
    capture=capture_valid(out,m["profile"])
    captured=capture["observation"]
    need(commissioner["head_at_compile"]==target["head_at_compile"] and
         m["commissioner_build_id"]==commissioner["build_id"]==capture["commissioner_build_id"] and
         m["target_build_id"]==target["build_id"],"ARTIFACT_CAPTURE_BINDING")
    need(request["plan"]["profile"]==m["profile"] and request["plan"]["partition_sha256"]==LAYOUT and
         all(request["plan"][key]==captured[key] for key in
             ("nvs_sha256","data_sha256","other_nvs_sha256","other_files_sha256")) and
         request["plan"]["backup_confirmed"] is True and
         request["plan"]["allow_blank_init"]==(captured["filesystem"]=="blank"),"PLAN_CAPTURE_BINDING")
    need(request["plan"]["target_sha256"]==image["sha256"] and
         request["plan"]["target_bytes"]==image["bytes"] and
         request["plan"]["target_build_id"]==target["build_id"],"PLAN_TARGET_BINDING")
    return out,m,request

class Transport:
    def __init__(self,port:str,build:str):
        try:import serial
        except ImportError:raise Stop("PYSERIAL_REQUIRED",4) from None
        self.s=serial.Serial(port=None,baudrate=115200,timeout=.1,write_timeout=5,
                             xonxoff=False,rtscts=False,dsrdtr=False)
        self.s.dtr=False;self.s.rts=False;self.s.port=port
        try:
            self.s.open()
            self.send(b"PING\n")
            need(self.line("PONG BOOTSTRAP_USB ")=="PONG BOOTSTRAP_USB "+build,"COMMISSIONER_IDENTITY",3)
        except BaseException:
            self.s.close()
            raise
    def close(self):self.s.close()
    def send(self,b):
        end=time.monotonic()+10
        while b:
            need(time.monotonic()<end,"SERIAL_WRITE_UNKNOWN",5)
            n=self.s.write(b);need(n>0,"SERIAL_WRITE_UNKNOWN",5);b=b[n:]
    def read(self,n):
        end=time.monotonic()+15;b=bytearray()
        while len(b)<n:
            need(time.monotonic()<end,"SERIAL_READ_UNKNOWN",5)
            b.extend(self.s.read(n-len(b)))
        return bytes(b)
    def line(self,prefix,timeout=30):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            b=bytearray()
            while len(b)<=4096:
                need(time.monotonic()<end,"SERIAL_REPLY_UNKNOWN",5)
                c=self.s.read(1)
                if c==b"\n":break
                b.extend(c)
            need(len(b)<=4096,"SERIAL_LINE_BOUND",5)
            line=b.rstrip(b"\r").decode("ascii",errors="replace")
            if line.startswith("ERR "):raise Stop("DEVICE_COMMAND_HOLD",5)
            if line.startswith(prefix):return line
        raise Stop("SERIAL_REPLY_UNKNOWN",5)
    def status(self,profile):
        self.send(("COMM_STATUS "+profile+"\n").encode())
        return load_unique(self.line("COMMISSION ").removeprefix("COMMISSION ").encode())
    def backup(self,kind,path,n,expected):
        # Finish access setup before asking the peer for its first block.
        private_acl.write_new(path,b"")
        self.send(("COMM_BACKUP "+kind+"\n").encode())
        line=self.line("COMM_BACKUP_READY ")
        fields=dict(p.split("=",1) for p in line.split()[1:] if "=" in p)
        need(fields=={"kind":kind,"length":str(n),"sha256":expected,"chunk":"1024"},"BACKUP_HEADER")
        with path.open("r+b") as f:
            done=0
            while done<n:
                b=self.read(min(1024,n-done));f.write(b);done+=len(b)
                self.send(("ACK "+str(done)+"\n").encode())
            f.flush();os.fsync(f.fileno())
        tail=self.line("COMM_BACKUP_OK ")
        need(tail=="COMM_BACKUP_OK length="+str(n)+" sha256="+expected and digest(path.read_bytes())==expected,
             "BACKUP_DIGEST")
        private_acl.verify(path)
    def frame(self,request):
        data=json.dumps(request,separators=(",",":"),ensure_ascii=False).encode()
        need(len(data)<=12288,"COMMISSION_FRAME_BOUND")
        self.send(("COMMISSION "+str(len(data))+" "+digest(data)+"\n").encode())
        need(self.line("COMMISSION_READY")=="COMMISSION_READY","COMMISSION_FRAME_READY")
        for at in range(0,len(data),1024):
            part=data[at:at+1024];self.send(part)
            need(self.line("COMM_ACK ")=="COMM_ACK "+str(at+len(part)),"COMMISSION_FRAME_ACK")
        return load_unique(self.line("COMMISSION ").removeprefix("COMMISSION ").encode())

def capture(a):
    commissioner,unused=app(a.commissioner_manifest,"commissioner")
    if a.dry_run:
        print(json.dumps({"result":"DRY_RUN","command":"capture","opens_serial":False}));return
    need(a.execute_read,"READ_EXECUTION_FLAG_REQUIRED",3)
    out=common.private(a.out);private_acl.directory(out)
    t=None
    try:
        t=Transport(a.port,commissioner["build_id"])
        before=t.status(a.profile)
        need(before["result"]=="OK","COMMISSIONER_PREFLIGHT")
        put(out/"snapshot-before.json",before)
        t.backup("nvs",out/"backup-nvs.bin",0x5000,before["nvs_sha256"])
        t.backup("data",out/"backup-data.bin",0x160000,before["data_sha256"])
        after=t.status(a.profile)
        need(after==before,"CAPTURE_CHANGED")
        doc={"profile":a.profile,"complete":True,"commissioner_build_id":commissioner["build_id"],"observation":before}
        put(out/"capture.json",doc)
        print(json.dumps({"result":"CAPTURED","out":str(out),"write_effects":"none",
                          "capture_sha256":digest((out/"capture.json").read_bytes())}))
    finally:
        if t:t.close()

def execute(a,verify=False):
    out,m,request=validate(a)
    if a.dry_run:
        print(json.dumps({"result":"DRY_RUN","command":"verify" if verify else "execute","opens_serial":False}));return
    if not verify:
        need(a.execute_write,"WRITE_EXECUTION_FLAG_REQUIRED",3)
        approval=load_unique(read_private(a.approval))
        need(approval.get("approve") is True and
             approval.get("allow_blank_init") is request["plan"]["allow_blank_init"] and
             approval=={"transaction_id":request["plan"]["transaction_id"],
            "request_sha256":m["request_sha256"],"allow_blank_init":request["plan"]["allow_blank_init"],
            "approve":True},"EXACT_OWNER_APPROVAL_REQUIRED",3)
    else:need(a.execute_read,"READ_EXECUTION_FLAG_REQUIRED",3)
    receipt=common.private(a.receipt)
    need(not receipt.exists(),"RECEIPT_EXISTS",3)
    private_acl.directory(receipt)
    if not verify:
        # Exclusive local transaction marker blocks blind repeat attempts even
        # if the process dies or the serial receipt is lost after mutation.
        marker=out/"execution-attempt.json"
        need(not marker.exists(),"PRIOR_EXECUTION_INSPECT_FIRST",3)
        put(marker,{"phase":"POSSIBLE_MUTATION_PENDING","request_sha256":m["request_sha256"],
                    "transaction_id":request["plan"]["transaction_id"]})
    t=None
    try:
        t=Transport(a.port,m["commissioner_build_id"])
        if verify:
            frame={"action":"verify","plan":request["plan"]}
        else:
            frame=dict(request);frame["plan"]={**request["plan"],"approved":True}
            put(receipt/"transaction-before.json",{"phase":"POSSIBLE_MUTATION_PENDING","request_sha256":m["request_sha256"],
                "transaction_id":request["plan"]["transaction_id"]})
        reply=t.frame(frame)
        put(receipt/"device-reply.json",reply)
        p=request["plan"]
        checked_reply(reply,p)
        put(receipt/"transaction-complete.json",{"result":"VERIFIED","transaction_id":p["transaction_id"],
            "profile_sha256":p["profile_sha256"],"journal_sha256":p["journal_sha256"]})
        print(json.dumps({"result":"VERIFIED","profile":m["profile"],"no_automatic_boot_or_start":True}))
    finally:
        if t:t.close()

def checked_reply(reply,p):
    need(reply["result"]=="OK","DEVICE_PARTIAL_OR_CONFLICT_HOLD",5)
    need(reply["transaction_id"]==p["transaction_id"] and reply["layout_verified"] is True and
         reply["namespace_absent"] is False and reply["journal_absent"] is False and
         reply["filesystem"]=="verified","READBACK_TRANSACTION_MISMATCH",5)
    need(all(reply[key]==p[key] for key in
         ("profile_sha256","journal_sha256","partition_sha256","other_nvs_sha256","other_files_sha256")) and
         all(re.fullmatch("[a-f0-9]{64}",reply[key]) for key in ("nvs_sha256","data_sha256")) and
         reply["data_sha256"]!=p["data_sha256"],
         "READBACK_PRESERVATION_HOLD",5)
    if p["profile"]=="lab":
        need(reply["nvs_sha256"]==p["nvs_sha256"],"LAB_NVS_CHANGED_HOLD",5)

def finalize(a):
    out,m,request=validate(a)
    if a.dry_run:
        print('{"result":"DRY_RUN","command":"finalize","opens_serial":false}');return
    need(a.execute_write,"WRITE_EXECUTION_FLAG_REQUIRED",3)
    target,image=app(out/"target-app-manifest.json",m["profile"])
    approval=load_unique(read_private(a.approval))
    need(approval.get("approve") is True and
         approval=={"transaction_id":request["plan"]["transaction_id"],"action":"flash-target-app",
                    "target_sha256":image["sha256"],"approve":True},"EXACT_TARGET_APPROVAL_REQUIRED",3)
    data=(ROOT/image["path"]).read_bytes()
    need(len(data)==image["bytes"] and digest(data)==image["sha256"],"ARTIFACT_DIGEST")
    receipt=common.private(a.receipt);need(not receipt.exists(),"RECEIPT_EXISTS",3)
    private_acl.directory(receipt)
    marker=out/"finalize-attempt.json"
    need(not marker.exists(),"PRIOR_FINALIZE_INSPECT_FIRST",3)
    put(marker,{"phase":"POSSIBLE_ACTIVATION_PENDING","target_sha256":image["sha256"]})
    t=None
    try:
        t=Transport(a.port,m["commissioner_build_id"])
        verified=t.frame({"action":"verify","plan":request["plan"]})
        put(receipt/"device-reply.json",verified)
        checked_reply(verified,request["plan"])
        t.send(b"INFO\n");info=t.line("INFO BOOTSTRAP_USB ")
        fields=dict(x.split("=",1) for x in info.split() if "=" in x)
        need(info.split()[2]==m["commissioner_build_id"] and fields.get("protocol")=="1" and
             fields.get("slot_bytes")=="1310720" and fields.get("chunk")=="1024" and
             fields.get("running") in ("app0","app1") and fields.get("target") in ("app0","app1") and
             fields["running"]!=fields["target"],"UPDATE_SLOT_GUARD",5)
        put(receipt/"transaction-before.json",{"phase":"POSSIBLE_ACTIVATION_PENDING","target_sha256":image["sha256"]})
        t.send(("BEGIN "+str(image["bytes"])+" "+image["sha256"]+"\n").encode())
        ready=t.line("READY ")
        got=dict(x.split("=",1) for x in ready.split()[1:] if "=" in x)
        need(got.get("length")==str(len(data)) and got.get("target")==fields["target"] and
             got.get("chunk")=="1024","UPDATE_READY_MISMATCH",5)
        for at in range(0,len(data),1024):
            part=data[at:at+1024];t.send(part)
            need(t.line("ACK ")=="ACK "+str(at+len(part)),"UPDATE_ACK_UNKNOWN",5)
        final=t.line("OK ")
        got=dict(x.split("=",1) for x in final.split() if "=" in x)
        need(got=={"length":str(len(data)),"sha256":image["sha256"],"from":fields["running"],
                   "target":fields["target"],"restarting":"1"},"ACTIVATION_RECEIPT_UNKNOWN",5)
        put(receipt/"activation-receipt.json",{"receipt":got,"build_id":target["build_id"]})
        if m["profile"]=="lab":
            marker=t.line("BOOTSTRAP_USB ",timeout=20)
            need(marker.startswith("BOOTSTRAP_USB "+target["build_id"]+" "),"TARGET_BOOT_UNKNOWN",5)
            t.send(b"PING\n")
            need(t.line("PONG BOOTSTRAP_USB ")=="PONG BOOTSTRAP_USB "+target["build_id"],"TARGET_BOOT_UNKNOWN",5)
        else:
            # Release deliberately has no updater/command parser. Wait for its
            # output-only periodic diagnostic instead of expecting a PONG.
            marker=t.line("MG build="+target["build_id"]+" profile=release ",timeout=45)
        put(receipt/"boot-receipt.json",{"build_id":target["build_id"],"observed":True,
                                       "profile":m["profile"],"watering_acceptance":False})
        print(json.dumps({"result":"TARGET_BOOT_OBSERVED","profile":m["profile"],"watering_acceptance":False}))
    finally:
        if t:t.close()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--version",action="version",version="miGarden current commissioner v1")
    sub=p.add_subparsers(dest="command",required=True)
    cap=sub.add_parser("capture",help="Owner-run read-only full private NVS/data backups from pinned commissioner")
    cap.add_argument("--commissioner-manifest",type=Path,required=True)
    cap.add_argument("--profile",choices=common.PROFILES,required=True)
    cap.add_argument("--port",required=True);cap.add_argument("--out",type=Path,required=True)
    cap.add_argument("--execute-read",action="store_true");cap.add_argument("--dry-run",action="store_true")
    prep=sub.add_parser("prepare",help="Offline preserving plan from complete private backup capture")
    for name in ("capture","device-binding","app-manifest","commissioner-manifest","sim","out"):
        prep.add_argument("--"+name,type=Path,required=True)
    prep.add_argument("--private-config",required=True)
    prep.add_argument("--capture-sha256",required=True,help="Separately retained complete capture receipt digest")
    prep.add_argument("--profile",choices=common.PROFILES,required=True)
    prep.add_argument("--genesis",type=Path,required=True)
    prep.add_argument("--sim-sha256",required=True)
    prep.add_argument("--nvs-parser",type=Path,default=common.DEFAULT_PARSER)
    prep.add_argument("--dry-run",action="store_true")
    check=sub.add_parser("validate",help="Offline exact-file-set, pin, profile/binding and backup validation")
    check.add_argument("--bundle",type=Path,required=True);check.add_argument("--manifest-sha256",required=True)
    for cmd in ("execute","verify","finalize"):
        x=sub.add_parser(cmd,help="Owner-run single transaction or read-only verification; no automatic retry")
        x.add_argument("--bundle",type=Path,required=True);x.add_argument("--manifest-sha256",required=True)
        x.add_argument("--port",required=True);x.add_argument("--receipt",type=Path,required=True)
        x.add_argument("--dry-run",action="store_true")
        if cmd in ("execute","finalize"):
            x.add_argument("--approval",type=Path,required=True);x.add_argument("--execute-write",action="store_true")
        else:x.add_argument("--execute-read",action="store_true")
    a=p.parse_args()
    try:
        if a.command=="prepare":
            prepare(a)
        elif a.command=="capture":capture(a)
        elif a.command=="validate":validate(a);print('{"result":"VALID","execution_granted":false}')
        elif a.command=="finalize":finalize(a)
        else:execute(a,a.command=="verify")
        return 0
    except Stop as e:print("ERROR "+e.code,file=sys.stderr);return e.status
    except private_acl.PrivacyError:print("ERROR PRIVATE_ACCESS_REQUIRED",file=sys.stderr);return 3
    except KeyboardInterrupt:print("INTERRUPTED; inspect any attempt receipt before retry",file=sys.stderr);return 130
    except Exception:
        print("ERROR PRIVATE_INPUT_OR_TRANSPORT; no values displayed; inspect receipt before retry",file=sys.stderr)
        return 5
if __name__=="__main__":raise SystemExit(main())
