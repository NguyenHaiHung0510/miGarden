"""Prepare or verify an OFFLINE first-commissioning bundle. Never opens a device."""
from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import importlib.util
import json
from pathlib import Path
import re
import ssl
import subprocess
import sys
import tempfile
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[2]
PARSER_SHA = "621bdbf0ac60e34ae190f63be10f0f1a4dd4d18c25ebab0cf56ff53a6f2b6c2c"
NVS_BYTES, DATA_BYTES = 0x5000, 0x160000
OFFSETS = {"nvs": 0x9000, "data": 0x290000}
PROFILES = {"release": ("mg-rel-v1", "release-v1.journal"),
            "lab": ("mg-lab-v1", "lab-v1.journal")}
DEFAULT_MKFS = Path(r"C:\Users\os\AppData\Local\Arduino15\packages\esp32\tools\mklittlefs\4.0.2-db0513a\mklittlefs.exe")
DEFAULT_PARSER = ROOT / ".local/n01/deps/nvs-tool-v5.5.5/nvs_parser.py"


class Stop(Exception):
    def __init__(self, code: str, status: int = 2):
        self.code, self.status = code, status


def need(ok, code: str, status: int = 2):
    if not ok:
        raise Stop(code, status)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_unique(data: bytes):
    def pairs(items):
        out = {}
        for k, v in items:
            need(k not in out, "DUPLICATE_JSON_KEY")
            out[k] = v
        return out
    try:
        return json.loads(data.decode("utf-8"), object_pairs_hook=pairs)
    except (UnicodeError, ValueError):
        raise Stop("INVALID_PRIVATE_JSON") from None


def profile_bytes(data: bytes, profile: str) -> bytes:
    need(len(data) <= 8192 and b"\0" not in data, "INVALID_PROFILE_SIZE")
    p = load_unique(data)
    required = {"device_id", "wifi_ssid", "wifi_password", "base_url", "device_token", "ca_pem"}
    need(isinstance(p, dict) and required <= p.keys() and p.keys() <= required | {"fixture_verified"},
         "INVALID_PROFILE_KEYS")
    for key in required:
        need(isinstance(p[key], str) and "\0" not in p[key], "INVALID_PROFILE_FIELD")
    need(bool(re.fullmatch(r"[!-~]{1,128}", p["device_id"])), "INVALID_DEVICE_ID")
    need(0 < len(p["wifi_ssid"].encode()) <= 32 and len(p["wifi_password"].encode()) <= 64, "INVALID_WIFI_SIZE")
    pw = p["wifi_password"].encode()
    need(not pw or 8 <= len(pw) < 64 or len(pw) == 64 and bool(re.fullmatch(rb"[0-9a-fA-F]{64}", pw)),
         "INVALID_WIFI_PASSWORD")
    url = urlsplit(p["base_url"])
    try:
        port = url.port
    except ValueError:
        raise Stop("INVALID_HTTPS_ORIGIN") from None
    need(len(p["base_url"].encode()) <= 256 and url.scheme == "https" and bool(url.hostname) and
         bool(re.fullmatch(r"[A-Za-z0-9.-]+", url.hostname or "")) and
         not url.username and not url.password and url.path in ("", "/") and
         not url.query and not url.fragment and (port is None or 0 < port <= 65535), "INVALID_HTTPS_ORIGIN")
    need(bool(re.fullmatch(r"[A-Za-z0-9._~+/=-]{32,256}", p["device_token"])), "INVALID_TOKEN")
    ca = p["ca_pem"].encode()
    need(len(ca) <= 6144 and b"PRIVATE KEY" not in ca and b"BEGIN CERTIFICATE" in ca, "INVALID_CA")
    try:
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(cadata=p["ca_pem"])
    except (ValueError, ssl.SSLError):
        raise Stop("INVALID_CA") from None
    need(type(p.get("fixture_verified", False)) is bool, "INVALID_FIXTURE_FLAG")
    need(profile != "release" or p.get("fixture_verified") is True, "FIXTURE_REVIEW_REQUIRED", 3)
    out = json.dumps(p, ensure_ascii=False, separators=(",", ":")).encode()
    need(len(out) <= 8192, "INVALID_PROFILE_SIZE")
    return out


def private(path: Path):
    path = path.resolve()
    need(".local" in path.parts or "provisioning-private" in path.parts, "PRIVATE_DESTINATION_REQUIRED")
    return path


def blank(path: Path, n: int, code: str):
    data = path.read_bytes()
    need(len(data) == n, "SNAPSHOT_SIZE_MISMATCH")
    need(data == b"\xff" * n, code, 3)
    return {"bytes": n, "sha256": digest(data)}


def run(args: list, cwd: Path):
    r = subprocess.run([str(a) for a in args], cwd=cwd, capture_output=True, timeout=120)
    # Child tools can print values or paths. Never echo their output/error.
    need(r.returncode == 0, "OFFLINE_TOOL_FAILED", 4)


def parser_at(path: Path):
    need(digest(path.read_bytes()) == PARSER_SHA, "NVS_PARSER_VERSION_MISMATCH", 4)
    spec = importlib.util.spec_from_file_location("commission_nvs_parser", path)
    need(spec and spec.loader, "NVS_PARSER_UNAVAILABLE", 4)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_nvs(path: Path, expected: bytes, namespace: str, parser: Path):
    raw = path.read_bytes()
    need(len(raw) == NVS_BYTES, "NVS_IMAGE_SIZE")
    parsed = parser_at(parser).NVS_Partition("private-bundle", bytearray(raw))
    written = []
    for page in parsed.pages:
        if page.header["status"] != "Empty":
            need(page.header["status"] in ("Active", "Full") and
                 page.header["crc"]["original"] == page.header["crc"]["computed"], "NVS_HEADER_CRC")
        for entry in page.entries:
            if entry.state == "Written":
                c = entry.metadata["crc"]
                need(c["original"] == c["computed"], "NVS_ENTRY_CRC")
                if entry.metadata["type"] == "blob_data":
                    need(c["data_original"] == c["data_computed"], "NVS_BLOB_CRC")
                written.append(entry)
    ns = [e for e in written if e.metadata["namespace"] == 0]
    need(len(ns) == 1 and ns[0].key == namespace and ns[0].metadata["type"] == "uint8_t", "NVS_NAMESPACE")
    i = ns[0].data["value"]
    items = [e for e in written if e.metadata["namespace"] != 0]
    need(all(e.metadata["namespace"] == i and e.key == "profile-v1" for e in items), "NVS_KEYS")
    chunks = sorted((e for e in items if e.metadata["type"] == "blob_data"),
                    key=lambda e: e.metadata["chunk_index"])
    indices = [e for e in items if e.metadata["type"] == "blob_index"]
    need(len(indices) == 1 and indices[0].data["size"] == len(expected) and
         indices[0].data["chunk_count"] == len(chunks) and len(items) == len(chunks) + 1, "NVS_INDEX")
    recovered = b"".join(b"".join(bytes(child.raw) for child in e.children)[:e.data["size"]] for e in chunks)
    need(recovered == expected, "NVS_PROFILE_ROUNDTRIP")


def check_app(manifest: Path, profile: str, config: bytes):
    m = load_unique(manifest.read_bytes())
    need(m["artifact_gate"] == "PASS" and m["compile_exit"] == 0 and m["profile"] == profile and
         m["wire"] == "migarden.zone-a.v1" and m["configured_flash_bytes"] == 0x400000 and
         m["app0"] == {"start": 0x10000, "bytes": 0x140000} and
         m["app1"] == {"start": 0x150000, "bytes": 0x140000} and
         m["data"]["start"] == OFFSETS["data"] and m["data"]["bytes"] == DATA_BYTES, "APP_LAYOUT_MISMATCH")
    images = [a for a in m["artifacts"] if a["path"].endswith("MiGardenController.ino.bin")]
    need(len(images) == 1, "APP_IMAGE_MISSING")
    image = ROOT / images[0]["path"]
    body = image.read_bytes()
    need(len(body) == images[0]["bytes"] and digest(body) == images[0]["sha256"], "APP_HASH_MISMATCH")
    p = load_unique(config)
    need(p["device_token"].encode() not in body, "CREDENTIAL_IN_APPLICATION")
    return {"head": m["head_at_compile"], "build_id": m["build_id"], "sha256": images[0]["sha256"]}


def prepare(a):
    out = private(a.out)
    need(not out.exists(), "DESTINATION_EXISTS", 3)
    need(a.private_config == "-" or ".local" in Path(a.private_config).resolve().parts or
         "provisioning-private" in Path(a.private_config).resolve().parts, "PRIVATE_INPUT_REQUIRED")
    data = sys.stdin.buffer.read(8193) if a.private_config == "-" else Path(a.private_config).read_bytes()
    profile = profile_bytes(data, a.profile)
    evidence = {
        "nvs": blank(a.nvs_before, NVS_BYTES, "EXISTING_NVS_HOLD"),
        "data": blank(a.data_before, DATA_BYTES, "EXISTING_DATA_HOLD"),
    }
    app = check_app(a.app_manifest, a.profile, profile)
    namespace, journal = PROFILES[a.profile]
    plan = {"mode": "OFFLINE_ONLY", "profile": a.profile, "namespace": namespace, "key": "profile-v1",
            "journal": journal, "fixture_verified": a.profile == "release", "write_grant": False,
            "app": app, "preimage": evidence, "scope": {"nvs": [0x9000, NVS_BYTES], "data": [0x290000, DATA_BYTES]}}
    if a.dry_run:
        print(json.dumps({"result": "DRY_RUN", **plan}, separators=(",", ":")))
        return
    need(importlib.metadata.version("esp-idf-nvs-partition-gen") == "0.3.0", "NVS_GENERATOR_VERSION", 4)
    parser_at(a.nvs_parser)
    need(a.mkfs.is_file() and a.sim.is_file(), "OFFLINE_TOOL_MISSING", 4)
    # Exclusive new bundle; no existing file/namespace/journal can be overwritten.
    out.mkdir(parents=True, exist_ok=False)
    (out / "blank").mkdir()
    (out / "unpacked").mkdir()
    (out / "profile.json").write_bytes(profile)
    with (out / "nvs.csv").open("x", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerows([["key", "type", "encoding", "value"],
                         [namespace, "namespace", "", ""],
                         ["profile-v1", "file", "binary", "profile.json"]])
    run([sys.executable, "-m", "esp_idf_nvs_partition_gen", "generate", "--version", "2",
         "nvs.csv", "profile-nvs.bin", hex(NVS_BYTES)], out)
    check_nvs(out / "profile-nvs.bin", profile, namespace, a.nvs_parser)
    run([a.sim.resolve(), "genesis", out / "blank" / journal], out)
    run([a.mkfs.resolve(), "-c", "blank", "-p", "256", "-b", "4096", "-s", str(DATA_BYTES),
         "data-genesis.bin"], out)
    run([a.mkfs.resolve(), "-u", "unpacked", "-p", "256", "-b", "4096", "-s", str(DATA_BYTES),
         "data-genesis.bin"], out)
    need((out / "blank" / journal).read_bytes() == (out / "unpacked" / journal).read_bytes(),
         "GENESIS_ROUNDTRIP")
    files = {}
    for f in sorted(out.rglob("*")):
        if f.is_file():
            b = f.read_bytes()
            files[f.relative_to(out).as_posix()] = {"bytes": len(b), "sha256": digest(b)}
    m = {**plan, "created_at": datetime.now(timezone.utc).isoformat(), "files": files,
         "nvs_parser_sha256": PARSER_SHA, "generator": "esp-idf-nvs-partition-gen==0.3.0"}
    (out / "manifest.json").write_text(json.dumps(m, indent=2) + "\n", encoding="utf-8")
    (out / "OWNER-APPLY.txt").write_text(
        "REVIEW ONLY. No write grant is conveyed by this bundle.\n"
        "Use only first commissioning with freshly rechecked full blank NVS and data evidence.\n"
        "This ROM write erases/programs exactly NVS0x9000/0x5000 and data0x290000/0x160000.\n"
        "Never use on a provisioned board, an unknown partition or an existing journal.\n"
        "No whole-chip erase, --erase-all, partition table, bootloader or efuse change.\n"
        "The retained app USB v1 cannot write these data files. A separately authorized ROM-loader session is required.\n"
        "After Owner approval and device preflight, from this private bundle directory:\n"
        "  <OWNER_PYTHON> -m esptool --chip esp32 --port <LEASED_PORT> --baud 115200 "
        "--before no-reset --after no-reset write-flash --flash-mode keep --flash-freq keep --flash-size keep "
        "0x9000 profile-nvs.bin 0x290000 data-genesis.bin\n"
        "If either region is nonblank, stop. Do not adapt this command to erase existing state.\n"
        "After authorized transfer, read back both ranges privately and compare hashes before normal boot.\n"
        "Flash only the separately pinned app by its approved method; verify build/profile, "
        "journal, trust clock/config and output fixture separately before enable.\n", encoding="utf-8")
    print(json.dumps({"result": "PREPARED", "out": str(out), "profile": a.profile, "write_grant": False}))


def validate(a):
    out = private(a.bundle)
    m = load_unique((out / "manifest.json").read_bytes())
    need(m["mode"] == "OFFLINE_ONLY" and m["write_grant"] is False, "INVALID_BUNDLE")
    need(m["profile"] in PROFILES, "INVALID_PROFILE")
    for name, entry in m["files"].items():
        p = (out / name).resolve()
        need(p.is_relative_to(out) and p.is_file(), "BUNDLE_PATH")
        raw = p.read_bytes()
        need(len(raw) == entry["bytes"] and digest(raw) == entry["sha256"], "BUNDLE_HASH")
    profile = (out / "profile.json").read_bytes()
    need(profile_bytes(profile, m["profile"]) == profile, "PROFILE_CANONICAL")
    namespace, journal = PROFILES[m["profile"]]
    check_nvs(out / "profile-nvs.bin", profile, namespace, a.nvs_parser)
    need((out / "blank" / journal).read_bytes() == (out / "unpacked" / journal).read_bytes(), "GENESIS_ROUNDTRIP")
    need((out / "data-genesis.bin").stat().st_size == DATA_BYTES, "FILESYSTEM_IMAGE_SIZE")
    print(json.dumps({"result": "VALID", "profile": m["profile"], "write_grant": False}))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--version", action="version", version="miGarden commissioning v1")
    sub = p.add_subparsers(dest="command", required=True)
    prep = sub.add_parser("prepare", help="Create a new private offline bundle from blank-region evidence")
    prep.add_argument("--profile", choices=PROFILES, required=True)
    prep.add_argument("--private-config", required=True, metavar="FILE_OR_DASH", help="Private JSON file or stdin; never secret values")
    prep.add_argument("--nvs-before", type=Path, required=True, help="Private exact 0x5000-byte blank readback")
    prep.add_argument("--data-before", type=Path, required=True, help="Private exact 0x160000-byte blank readback")
    prep.add_argument("--app-manifest", type=Path, required=True)
    prep.add_argument("--sim", type=Path, required=True)
    prep.add_argument("--mkfs", type=Path, default=DEFAULT_MKFS)
    prep.add_argument("--nvs-parser", type=Path, default=DEFAULT_PARSER)
    prep.add_argument("--out", type=Path, required=True, help="New private directory; no overwrite")
    prep.add_argument("--dry-run", action="store_true", help="Check and print redacted plan; create no files")
    check = sub.add_parser("validate", help="Verify private bundle bytes and NVS profile roundtrip without a device")
    check.add_argument("--bundle", type=Path, required=True)
    check.add_argument("--nvs-parser", type=Path, default=DEFAULT_PARSER)
    a = p.parse_args()
    try:
        if a.command == "prepare":
            prepare(a)
        else:
            validate(a)
        return 0
    except Stop as e:
        print("ERROR " + e.code, file=sys.stderr)
        return e.status
    except (OSError, ValueError, KeyError, TypeError, ImportError, importlib.metadata.PackageNotFoundError,
            subprocess.SubprocessError):
        print("ERROR OFFLINE_INPUT_OR_TOOL_FAILURE; private values suppressed", file=sys.stderr)
        return 4


if __name__ == "__main__":
    raise SystemExit(main())
