"""Two source-review guards against preserved ea140ed and the new C++ simulator."""
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
VECTORS = json.loads((ROOT / "contracts/zone-a/v1/vectors.json").read_text())
BASE = next(v["document"] for v in VECTORS if v["name"] == "desired")
CONFIG = BASE["desired_config"]


class Sim:
    def __init__(self, path):
        self.p = subprocess.Popen([str(path)], text=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def call(self, cmd):
        self.p.stdin.write(cmd + "\n")
        self.p.stdin.flush()
        return json.loads(self.p.stdout.readline())

    def exchange(self, **kwargs):
        return self.call("exchange " + json.dumps({**BASE, "desired_config": None, **kwargs}))

    def end(self):
        self.p.stdin.write("quit\n")
        self.p.stdin.flush()
        assert self.p.wait(timeout=10) == 0


def forward_reboot(path):
    s = Sim(path)
    c = {**CONFIG, "policy": {"manual_duration_seconds": 15, "max_session_seconds": 15,
                             "window_seconds": 10800, "max_window_seconds": 15},
         "schedule": {**CONFIG["schedule"], "duration_seconds": 15}}
    s.exchange(desired_config=c)
    s.call("start")
    s.call("advance 15000")
    s.call("jump 2026-11-18T04:00:15Z")
    s.call("reboot")
    s.call("clock synced")
    reply = s.call("start")
    s.end()
    return {"guard_pass": reply["snapshot"]["output"] == "off",
            "opens": reply["opens"], "reason": reply["snapshot"]["blocked_reason"]}


def full_stop_ack(path):
    s = Sim(path)
    ready = s.exchange(desired_config=CONFIG)
    intent = next(v["document"]["candidates"][0] for v in VECTORS if v["name"] == "candidate")
    for n in range(510):
        s.exchange(candidates=[{**intent, "request_id": "reject-" + str(n), "control_epoch": 900}])
    stop = {"kind": "stop", "zone_id": "zone-a", "binding_ref": "zone-a-v1",
            "boot_id": ready["boot_id"], "confirmed_at": BASE["server_time"], "request_id": "filler"}
    reply = s.exchange(stops=[stop])
    depth = s.call("diag")["queued"]
    assert depth == 512
    stop = {**stop, "request_id": "review-stop"}
    reply = s.exchange(stops=[stop], acked_event_ids=[e["event_id"] for e in reply["reports"]])
    reply = s.exchange(stops=[stop])
    found = False
    while reply["reports"]:
        found |= any(e["kind"] == "stop_processed" and e.get("request_id") == "review-stop"
                     for e in reply["reports"])
        reply = s.exchange(acked_event_ids=[e["event_id"] for e in reply["reports"]])
    epoch = reply["control_epoch"]
    s.end()
    return {"guard_pass": found, "epoch": epoch, "initial_queue": depth}


if __name__ == "__main__":
    old = ROOT / ".local/n01/host/sim.exe"
    new = Path(sys.argv[1])
    receipt = {}
    for label, path in (("ea140ed", old), ("revised", new)):
        receipt[label] = {"F1": forward_reboot(path), "F3": full_stop_ack(path)}
    print(json.dumps(receipt, indent=2))
    assert all(not case["guard_pass"] for case in receipt["ea140ed"].values())
    assert all(case["guard_pass"] for case in receipt["revised"].values())
