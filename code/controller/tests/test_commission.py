"""Isolated offline safety/roundtrip checks. No serial, credentials or real flash."""
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import secrets
import subprocess
import sys
import unittest
from uuid import uuid4

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID

ROOT = Path(__file__).resolve().parents[3]
CLI = ROOT / "code/controller/commission.py"


class Commission(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = ROOT / ".local/n01/commission-tests" / uuid4().hex
        cls.root.mkdir(parents=True)
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "synthetic offline fixture root")])
        now = datetime.now(timezone.utc)
        cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
                .serial_number(x509.random_serial_number()).not_valid_before(now - timedelta(days=1))
                .not_valid_after(now + timedelta(days=1)).add_extension(x509.BasicConstraints(ca=True, path_length=None),
                                                                          critical=True).sign(key, hashes.SHA256()))
        cls.p = {"device_id": "qa-offline-only", "wifi_ssid": "SYNTHETIC-NO-NETWORK",
                 "wifi_password": "synthetic-only-not-a-real-password", "base_url": "https://example.invalid:8765",
                 "device_token": secrets.token_urlsafe(32),
                 "ca_pem": cert.public_bytes(serialization.Encoding.PEM).decode(), "fixture_verified": True}
        cls.profile = cls.root / "profile.json"
        cls.profile.write_text(json.dumps(cls.p), encoding="utf-8")
        cls.nvs, cls.data = cls.root / "blank-nvs.bin", cls.root / "blank-data.bin"
        cls.nvs.write_bytes(b"\xff" * 0x5000)
        cls.data.write_bytes(b"\xff" * 0x160000)
        cls.sim = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / ".local/n01/host/sim.exe"
        cls.app = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / ".local/n01/build/n01_release_20261117T172909_3a779d65b198/manifest.json"

    def cmd(self, *args, stdin=None):
        r = subprocess.run([sys.executable, "-B", CLI, *map(str, args)], input=stdin,
                           text=True, capture_output=True, timeout=120)
        for secret in (self.p["device_token"], self.p["wifi_password"], self.p["wifi_ssid"], self.p["base_url"]):
            self.assertNotIn(secret, r.stdout + r.stderr)
        return r

    def base(self, name, **override):
        options = {"profile": "release", "private-config": self.profile, "nvs-before": self.nvs,
                   "data-before": self.data, "app-manifest": self.app, "sim": self.sim, "out": self.root / name}
        options.update(override)
        return ["prepare", *[x for key, value in options.items() for x in ("--" + key, str(value))]]

    def test_01_dry_run_has_no_outputs(self):
        r = self.cmd(*self.base("dry"), "--dry-run")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertFalse((self.root / "dry").exists())
        self.assertFalse(json.loads(r.stdout)["write_grant"])

    def test_02_nvs_profile_and_filesystem_roundtrip(self):
        r = self.cmd(*self.base("bundle"))
        self.assertEqual(r.returncode, 0, r.stderr)
        r = self.cmd("validate", "--bundle", self.root / "bundle")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual((self.root / "bundle/profile-nvs.bin").stat().st_size, 0x5000)
        self.assertEqual((self.root / "bundle/data-genesis.bin").stat().st_size, 0x160000)

    def test_03_never_overwrite_a_bundle(self):
        sentinel = self.root / "existing"
        sentinel.mkdir()
        (sentinel / "keep").write_bytes(b"preserve")
        r = self.cmd(*self.base("existing"))
        self.assertEqual(r.returncode, 3)
        self.assertEqual((sentinel / "keep").read_bytes(), b"preserve")

    def test_04_nonblank_nvs_and_journal_are_held(self):
        nvs = self.root / "existing-nvs.bin"
        nvs.write_bytes(b"x" + b"\xff" * (0x5000 - 1))
        r = self.cmd(*self.base("never-nvs", **{"nvs-before": nvs}))
        self.assertEqual(r.returncode, 3)
        self.assertIn("EXISTING_NVS_HOLD", r.stderr)
        self.assertFalse((self.root / "never-nvs").exists())
        data = self.root / "existing-data.bin"
        data.write_bytes(b"j" + b"\xff" * (0x160000 - 1))
        r = self.cmd(*self.base("never-data", **{"data-before": data}))
        self.assertEqual(r.returncode, 3)
        self.assertIn("EXISTING_DATA_HOLD", r.stderr)
        self.assertFalse((self.root / "never-data").exists())

    def test_05_fixture_not_assumed_for_release(self):
        path = self.root / "unverified.json"
        path.write_text(json.dumps({**self.p, "fixture_verified": False}), encoding="utf-8")
        r = self.cmd(*self.base("never-fixture", **{"private-config": path}))
        self.assertEqual(r.returncode, 3)
        self.assertIn("FIXTURE_REVIEW_REQUIRED", r.stderr)

    def test_06_stdin_and_missing_secrets_redacted(self):
        r = self.cmd(*self.base("stdin", **{"private-config": "-"}), "--dry-run", stdin=json.dumps(self.p))
        self.assertEqual(r.returncode, 0, r.stderr)
        r = self.cmd(*self.base("invalid", **{"private-config": "-"}), "--dry-run", stdin=self.p["device_token"])
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse((self.root / "invalid").exists())

    def test_07_guard_tamper_fails_then_restore_passes(self):
        r = self.cmd(*self.base("tamper"))
        self.assertEqual(r.returncode, 0, r.stderr)
        path = self.root / "tamper/profile-nvs.bin"
        old = path.read_bytes()
        path.write_bytes(b"\x00" + old[1:])
        failed = self.cmd("validate", "--bundle", self.root / "tamper")
        self.assertNotEqual(failed.returncode, 0)
        path.write_bytes(old)
        restored = self.cmd("validate", "--bundle", self.root / "tamper")
        self.assertEqual(restored.returncode, 0, restored.stderr)

    def test_08_help_no_effects(self):
        self.assertEqual(self.cmd("--help").returncode, 0)
        self.assertEqual(self.cmd("prepare", "--help").returncode, 0)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Commission)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    print("PRIVATE_TEST_ARTIFACTS", Commission.root)
    raise SystemExit(not result.wasSuccessful())
