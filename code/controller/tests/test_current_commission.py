"""Private synthetic current-layout plan tests; no serial or real profile."""
from argparse import Namespace
from contextlib import redirect_stdout
from datetime import datetime,timedelta,timezone
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch
from uuid import uuid4
from cryptography import x509
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/"code/controller"))
import commission
import commission_current as current
import private_acl
class Current(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root=ROOT/".local/n01/current-tests"/uuid4().hex
        private_acl.directory(cls.root)
        cls.sim=Path(sys.argv[1]).resolve()
        cls.cm=Path(sys.argv[2]).resolve()
        cls.rm=Path(sys.argv[3]).resolve()
        cls.lm=Path(sys.argv[4]).resolve()
        key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
        name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,"synthetic commissioner root")])
        now=datetime.now(timezone.utc)
        cert=(x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
              .serial_number(x509.random_serial_number()).not_valid_before(now-timedelta(days=1))
              .not_valid_after(now+timedelta(days=1)).add_extension(x509.BasicConstraints(ca=True,path_length=None),True)
              .sign(key,hashes.SHA256()))
        cls.profile={"device_id":"qa-preserve-test","wifi_ssid":"SYNTHETIC-ONLY","wifi_password":"synthetic-not-real",
            "base_url":"https://example.invalid:8765","device_token":"a"*43,
            "ca_pem":cert.public_bytes(serialization.Encoding.PEM).decode(),"fixture_verified":True}
        cls.payload=commission.profile_bytes(json.dumps(cls.profile).encode(),"release")
        cls.labStored=(json.dumps(cls.profile,indent=2)+"\n").encode()
        cls.profileFile=cls.root/"profile.json";private_acl.write_new(cls.profileFile,cls.payload)
        cls.binding=cls.root/"binding.json"
        private_acl.write_new(cls.binding,json.dumps({"protocol":"migarden.zone-a.v1","zone_id":"zone-a",
            "binding_ref":"zone-a-v1","device_id":cls.profile["device_id"],
            "token_hash":hashlib.sha256(cls.profile["device_token"].encode()).hexdigest(),
            "environment":"production","app_ref":"7b1d221"+"0"*33}).encode())
        cls.genesis=cls.root/"genesis.journal"
        subprocess.run([cls.sim,"genesis",cls.genesis],check=True,capture_output=True)
        private_acl.lock(cls.genesis)
        cls.capture=cls.root/"capture";private_acl.directory(cls.capture)
        # Build an existing LAB NVS through the official API-compatible generator.
        private_acl.write_new(cls.capture/"lab-profile.json",cls.labStored)
        private_acl.write_new(cls.capture/"nvs.csv",
            b"key,type,encoding,value\nmg-lab-v1,namespace,,\nprofile-v1,file,binary,lab-profile.json\n"
            b"other-key,data,u32,7\nother-ns,namespace,,\nother-key,data,u32,9\n")
        r=subprocess.run([sys.executable,"-m","esp_idf_nvs_partition_gen","generate","--version","2","nvs.csv",
            "backup-nvs.bin","0x5000"],cwd=cls.capture,capture_output=True)
        assert r.returncode==0
        private_acl.protect_generated(cls.capture)
        private_acl.write_new(cls.capture/"backup-data.bin",b"\xff"*0x160000)
        cm=json.loads(cls.cm.read_text())
        cls.observation={"result":"OK","layout_verified":True,"namespace_absent":True,"journal_absent":True,
            "filesystem":"blank","partition_sha256":current.LAYOUT,
            "nvs_sha256":hashlib.sha256((cls.capture/"backup-nvs.bin").read_bytes()).hexdigest(),
            "data_sha256":hashlib.sha256((cls.capture/"backup-data.bin").read_bytes()).hexdigest(),
            "other_nvs_sha256":"4"*64,"other_files_sha256":hashlib.sha256(b"").hexdigest(),
            "profile_sha256":"","journal_sha256":""}
        cls.captureDoc={"profile":"release","complete":True,"commissioner_build_id":cm["build_id"],"observation":cls.observation}
        private_acl.write_new(cls.capture/"capture.json",json.dumps(cls.captureDoc).encode())
    def command(self,*args):
        r=subprocess.run([sys.executable,"-B",ROOT/"code/controller/commission_current.py",*map(str,args)],
            text=True,capture_output=True,timeout=120)
        for s in (self.profile["device_token"],self.profile["wifi_password"],self.profile["wifi_ssid"],self.profile["base_url"]):
            self.assertNotIn(s,r.stdout+r.stderr)
        return r
    def prep(self,name="release",profile="release",capture=None,binding=None,target=None):
        cap=capture or self.capture
        return ["prepare","--capture",cap,"--capture-sha256",hashlib.sha256((cap/"capture.json").read_bytes()).hexdigest(),
                "--profile",profile,"--private-config",self.profileFile,
                "--device-binding",binding or self.binding,"--app-manifest",target or self.rm,
                "--commissioner-manifest",self.cm,"--sim",self.sim,"--sim-sha256",hashlib.sha256(self.sim.read_bytes()).hexdigest(),
                "--genesis",self.genesis,"--out",self.root/name]
    def test_01_existing_lab_nvs_release_preserving_plan(self):
        before=(self.capture/"backup-nvs.bin").read_bytes()
        r=self.command(*self.prep(),"--dry-run");self.assertEqual(r.returncode,0,r.stderr)
        self.assertFalse((self.root/"release").exists())
        r=self.command(*self.prep());self.assertEqual(r.returncode,0,r.stderr)
        out=self.root/"release";m=hashlib.sha256((out/"manifest.json").read_bytes()).hexdigest()
        r=self.command("validate","--bundle",out,"--manifest-sha256",m)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertEqual((self.capture/"backup-nvs.bin").read_bytes(),before)
        self.assertFalse(any(p.name.endswith(".bin") and p.name not in ("backup-nvs.bin","backup-data.bin") for p in out.iterdir()))
    def test_02_lab_plan_requires_exact_existing_profile_and_no_nvs_write(self):
        cap=self.root/"lab-capture";private_acl.directory(cap)
        for n in ("backup-nvs.bin","backup-data.bin"):private_acl.write_new(cap/n,(self.capture/n).read_bytes())
        doc=json.loads(json.dumps(self.captureDoc));doc["profile"]="lab"
        doc["observation"]["namespace_absent"]=False
        doc["observation"]["profile_sha256"]=hashlib.sha256(self.labStored).hexdigest()
        private_acl.write_new(cap/"capture.json",json.dumps(doc).encode())
        binding=json.loads(self.binding.read_bytes());binding["environment"]="qa"
        b=self.root/"lab-binding.json";private_acl.write_new(b,json.dumps(binding).encode())
        r=self.command(*self.prep("lab","lab",cap,b,self.lm))
        self.assertEqual(r.returncode,0,r.stderr)
        req=json.loads((self.root/"lab/request.json").read_bytes())
        self.assertEqual(req["plan"]["profile"],"lab")
        self.assertEqual(req["plan"]["profile_sha256"],hashlib.sha256(self.labStored).hexdigest())
        self.assertEqual((self.root/"lab/profile.json").read_bytes(),self.labStored)
    def test_03_existing_release_namespace_refuses(self):
        cap=self.root/"conflict";private_acl.directory(cap)
        for n in ("backup-nvs.bin","backup-data.bin"):private_acl.write_new(cap/n,(self.capture/n).read_bytes())
        doc=json.loads(json.dumps(self.captureDoc));doc["observation"]["namespace_absent"]=False
        private_acl.write_new(cap/"capture.json",json.dumps(doc).encode())
        r=self.command(*self.prep("refused",capture=cap))
        self.assertNotEqual(r.returncode,0)
        self.assertFalse((self.root/"refused").exists())
    def test_04_before_hash_and_bundle_tamper_refuse_restore(self):
        path=self.capture/"backup-data.bin";old=path.read_bytes()
        path.write_bytes(b"x"+old[1:])
        r=self.command(*self.prep("bad-hash"),"--dry-run");self.assertNotEqual(r.returncode,0)
        path.write_bytes(old)
        r=self.command(*self.prep("bad-hash"),"--dry-run");self.assertEqual(r.returncode,0,r.stderr)
        folder=self.root/"release";manifest=folder/"manifest.json";raw=manifest.read_bytes()
        m=json.loads(raw);m["files"].pop("backup-data.bin");manifest.write_text(json.dumps(m),encoding="utf-8")
        r=self.command("validate","--bundle",folder,"--manifest-sha256",hashlib.sha256(manifest.read_bytes()).hexdigest())
        self.assertNotEqual(r.returncode,0)
        manifest.write_bytes(raw)
        r=self.command("validate","--bundle",folder,"--manifest-sha256",hashlib.sha256(raw).hexdigest())
        self.assertEqual(r.returncode,0,r.stderr)
    def test_05_serial_frame_pacing_and_no_secret_echo(self):
        class Peer:
            def __init__(self):self.incoming=bytearray();self.payload=bytearray();self.expected=0;self.commands=[]
            def write(self,data):
                data=bytes(data);self.commands.append(data)
                if data.startswith(b"COMMISSION "):
                    self.expected=int(data.split()[1]);self.incoming.extend(b"COMMISSION_READY\n")
                else:
                    self.payload.extend(data);self.incoming.extend(("COMM_ACK "+str(len(self.payload))+"\n").encode())
                    if len(self.payload)==self.expected:self.incoming.extend(b'COMMISSION {"result":"OK"}\n')
                return len(data)
            def read(self,n=1):
                b=bytes(self.incoming[:n]);del self.incoming[:n];return b
        t=current.Transport.__new__(current.Transport);t.s=Peer()
        request={"action":"apply","profile_json":"x"*5000}
        reply=t.frame(request)
        self.assertEqual(reply,{"result":"OK"})
        self.assertEqual(json.loads(t.s.payload),request)
        self.assertTrue(all(len(chunk)<=1024 for chunk in t.s.commands[1:]))

    def operator(self,profile,verb):
        bundle=self.root/profile
        approval=json.loads((bundle/(("finalize" if verb=="finalize" else "owner")+"-approval.template.json")).read_bytes())
        approval["approve"]=True
        approvalPath=self.root/(verb+"-approval.json")
        private_acl.write_new(approvalPath,json.dumps(approval).encode())
        a=Namespace(bundle=bundle,manifest_sha256=current.digest((bundle/"manifest.json").read_bytes()),
                    dry_run=False,port="SYNTHETIC_PEER",execute_write=True,execute_read=True,
                    approval=approvalPath,receipt=self.root/(verb+"-receipt"))
        p=json.loads((bundle/"request.json").read_bytes())["plan"]
        reply={key:p[key] for key in ("transaction_id","partition_sha256","other_nvs_sha256",
               "other_files_sha256","profile_sha256","journal_sha256","nvs_sha256","data_sha256")}
        reply.update(result="OK",filesystem="verified",layout_verified=True,namespace_absent=False,journal_absent=False)
        reply["data_sha256"]="8"*64
        return a,p,reply

    def test_06_unapproved_or_lost_write_reply_does_not_retry(self):
        a,p,reply=self.operator("lab","execute")
        good=a.approval.read_bytes()
        wrong=json.loads(good);wrong["approve"]=1
        a.approval.write_text(json.dumps(wrong),encoding="utf-8")
        with patch.object(current,"Transport") as transport:
            with self.assertRaisesRegex(commission.Stop,"EXACT_OWNER_APPROVAL_REQUIRED"):current.execute(a)
            transport.assert_not_called()
            self.assertFalse(a.receipt.exists())
        a.approval.write_bytes(good)
        with patch.object(current,"Transport") as transport:
            transport.return_value.frame.side_effect=commission.Stop("SERIAL_REPLY_UNKNOWN",5)
            with self.assertRaisesRegex(commission.Stop,"SERIAL_REPLY_UNKNOWN"):current.execute(a)
            self.assertTrue(transport.return_value.frame.call_args.args[0]["plan"]["approved"])
        self.assertTrue((a.bundle/"execution-attempt.json").exists())
        self.assertFalse((a.receipt/"transaction-complete.json").exists())
        a.receipt=self.root/"inspect-instead-of-retry"
        with patch.object(current,"Transport") as transport:
            with self.assertRaisesRegex(commission.Stop,"PRIOR_EXECUTION_INSPECT_FIRST"):current.execute(a)
            transport.assert_not_called()
            a.receipt=self.root/"verify-receipt"
            transport.return_value.frame.return_value=reply
            output=io.StringIO()
            with redirect_stdout(output):current.execute(a,True)
            self.assertEqual(transport.return_value.frame.call_args.args[0],{"action":"verify","plan":p})
            self.assertEqual(json.loads(output.getvalue())["result"],"VERIFIED")

    def test_07_finalize_verified_release_exact_artifact(self):
        a,p,reply=self.operator("release","finalize")
        m=json.loads((a.bundle/"manifest.json").read_bytes())
        body=bytearray()
        initial={"info":False,"begin":False}
        def send(b):
            if b==b"INFO\n":
                initial["info"]=True
            elif b.startswith(b"BEGIN "):
                self.assertEqual(b,f'BEGIN {p["target_bytes"]} {p["target_sha256"]}\n'.encode())
                initial["begin"]=True
            else:
                self.assertTrue(initial["begin"] and 0<len(b)<=1024)
                body.extend(b)
        def line(prefix,timeout=30):
            if prefix=="INFO BOOTSTRAP_USB ":
                self.assertTrue(initial["info"])
                return prefix+m["commissioner_build_id"]+" protocol=1 slot_bytes=1310720 running=app0 target=app1 chunk=1024"
            if prefix=="READY ":return prefix+f'chunk=1024 length={p["target_bytes"]} target=app1'
            if prefix=="ACK ":return prefix+str(len(body))
            if prefix=="OK ":
                self.assertEqual(current.digest(body),p["target_sha256"])
                return prefix+f'length={len(body)} sha256={p["target_sha256"]} from=app0 target=app1 restarting=1'
            self.assertEqual(prefix,"MG build="+p["target_build_id"]+" profile=release ")
            return prefix+"output=off"
        with patch.object(current,"Transport") as transport:
            peer=transport.return_value
            peer.frame.return_value=reply;peer.send.side_effect=send;peer.line.side_effect=line
            output=io.StringIO()
            with redirect_stdout(output):current.finalize(a)
            self.assertEqual(peer.frame.call_args.args[0],{"action":"verify","plan":p})
        self.assertTrue((a.receipt/"activation-receipt.json").exists())
        self.assertFalse(json.loads((a.receipt/"boot-receipt.json").read_bytes())["watering_acceptance"])
        self.assertEqual(json.loads(output.getvalue())["result"],"TARGET_BOOT_OBSERVED")
if __name__=="__main__":
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(Current)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    print("PRIVATE_CURRENT_TEST_ARTIFACTS",Current.root)
    raise SystemExit(not result.wasSuccessful())
