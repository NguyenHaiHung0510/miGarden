"""Check the frozen shape and exercise the actual C++ simulator through stdio."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys

from jsonschema import Draft202012Validator, FormatChecker

ROOT = Path(__file__).resolve().parents[3]
WIRE = ROOT / "contracts" / "zone-a" / "v1"


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def run(sim: Path) -> None:
    schema = read_json(WIRE / "exchange.schema.json")
    Draft202012Validator.check_schema(schema)
    oracle = Draft202012Validator(schema, format_checker=FormatChecker())
    vectors = {v["name"]: v for v in read_json(WIRE / "vectors.json")}
    for name, v in vectors.items():
        assert oracle.is_valid(v["document"]) == v["valid"], name
    print(f"PASS schema vectors {len(vectors)}/{len(vectors)}")
    extended = Draft202012Validator(read_json(WIRE / "quiet-health.schema.json"), format_checker=FormatChecker())
    extended_vectors = read_json(WIRE / "quiet-health.vectors.json")
    for v in extended_vectors:
        assert extended.is_valid(v["document"]) == v["valid"], v["name"]
        assert oracle.is_valid(v["document"]) == v["legacy_valid"], v["name"] + " legacy"
    print(f"PASS quiet-health compatibility vectors {len(extended_vectors)}/{len(extended_vectors)}")
    docs = {k: v["document"] for k, v in vectors.items() if v["valid"]}

    with subprocess.Popen([sim], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          text=True, encoding="utf-8") as p:
        def call(command: str):
            assert p.stdin is not None and p.stdout is not None
            p.stdin.write(command + "\n")
            p.stdin.flush()
            reply = json.loads(p.stdout.readline())
            assert "error" not in reply, command
            return reply

        def xchg(**overrides):
            wire = {
                **docs["desired"],
                "desired_config": None,
                **overrides,
            }
            oracle.validate(wire)
            reply = call("exchange " + json.dumps(wire, separators=(",", ":")))
            oracle.validate(reply)
            return reply

        start = call("snapshot")
        oracle.validate(start)
        assert start["snapshot"]["ready"] is False
        config = xchg(desired_config=docs["desired"]["desired_config"])
        assert config["snapshot"]["ready"] is True
        assert config["reports"][0]["kind"] == "config_applied"
        intent = docs["candidate"]["candidates"][0]
        req = xchg(candidates=[intent])
        assert req["snapshot"]["ready"] is True
        receipt = next(r for r in req["reports"] if r["kind"] == "request_received")
        assert receipt["request_id"] == intent["request_id"]
        assert req["snapshot"]["active_session"] is None
        grants = [{**intent, "grant_id": "grant-1"}]
        working = xchg(grants=grants, acked_event_ids=[r["event_id"] for r in req["reports"]])
        session = working["snapshot"]["active_session"]
        assert session and session["trigger_source"] == "remote"
        assert working["snapshot"]["output"] == "on"
        call("advance 17000")
        stop = {
            "request_id": "stop-1", "kind": "stop", "zone_id": "zone-a",
            "binding_ref": "zone-a-v1", "boot_id": start["boot_id"],
            "confirmed_at": "2026-11-18T00:00:17Z",
        }
        done = xchg(stops=[stop], grants=grants)
        assert done["snapshot"]["output"] == "off" and done["control_epoch"] == 1
        end = next(r for r in done["reports"] if r["kind"] == "session_ended")
        assert end["session_id"] == session["session_id"]
        assert end["payload"]["outcome"] == "stopped"
        assert end["payload"]["output_duration_seconds"] == 17
        receipt = next(r for r in done["reports"] if r["kind"] == "stop_processed")
        assert receipt["request_id"] == "stop-1" and receipt["payload"]["outcome"] == "close_requested"
        same = xchg(stops=[stop], grants=grants)
        assert same["control_epoch"] == 1 and same["reports"] == done["reports"]
        ack = xchg(acked_event_ids=[r["event_id"] for r in done["reports"]])
        assert ack["reports"] == [] and not ack["snapshot"]["active_session"]
        call("reboot")
        recovered = call("snapshot")
        oracle.validate(recovered)
        assert recovered["boot_id"] != start["boot_id"] and recovered["reports"] == []
        assert recovered["snapshot"]["applied_config_ref"] == "cfg-1"
        assert recovered["snapshot"]["time_quality"] == "unknown"
        assert not recovered["snapshot"]["ready"]
        assert p.stdin is not None
        p.stdin.write("quit\n")
        p.stdin.flush()
        assert p.wait(timeout=10) == 0
    print("PASS simulator v1 roundtrip (config/receipt/grant/start/17s STOP/retry/ACK/reboot)")


if __name__ == "__main__":
    run(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / ".local/n01/host/sim.exe")
