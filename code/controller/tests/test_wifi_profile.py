"""Synthetic-only profile validation: no serial, files, endpoints or real credentials."""
from copy import deepcopy
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import sys
import unittest

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import commission


class WifiProfiles(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "synthetic wifi profile CA")])
        now = datetime.now(timezone.utc)
        cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
                .serial_number(x509.random_serial_number()).not_valid_before(now-timedelta(days=1))
                .not_valid_after(now+timedelta(days=1)).add_extension(x509.BasicConstraints(ca=True, path_length=None), True)
                .sign(key, hashes.SHA256()))
        cls.legacy = {"device_id": "synthetic-wifi-unit", "wifi_ssid": "SYNTHETIC preferred",
                      "wifi_password": "synthetic preferred pass", "base_url": "https://example.invalid:8765",
                      "device_token": "a"*43, "ca_pem": cert.public_bytes(serialization.Encoding.PEM).decode(),
                      "fixture_verified": True}
        cls.dual = {**cls.legacy, "wifi_fallback_ssid": "SYNTHETIC secondary",
                    "wifi_fallback_password": "synthetic secondary pass"}

    def valid(self, p):
        return commission.profile_bytes(json.dumps(p).encode(), "release")

    def refused_then_restored(self, p, code):
        with self.assertRaises(commission.Stop) as caught:
            self.valid(p)
        self.assertEqual(caught.exception.code, code)
        self.assertEqual(json.loads(self.valid(self.dual)), self.dual)

    def test_01_legacy_and_dual_preserve_unrelated_fields(self):
        for p in (self.legacy, self.dual):
            raw = self.valid(p)
            self.assertEqual(json.loads(raw), p)
            self.assertEqual(self.valid(json.loads(raw)), raw)
        for k in self.legacy:
            self.assertEqual(json.loads(self.valid(self.dual))[k], self.legacy[k])

    def test_02_pair_null_empty_and_duplicate_fail_restore(self):
        for key in ("wifi_fallback_ssid", "wifi_fallback_password"):
            p = deepcopy(self.dual); del p[key]
            self.refused_then_restored(p, "WIFI_FALLBACK_PAIR_REQUIRED")
            p = deepcopy(self.dual); p[key] = None
            self.refused_then_restored(p, "INVALID_WIFI_FIELD")
        p = deepcopy(self.dual); p["wifi_fallback_ssid"] = ""
        self.refused_then_restored(p, "INVALID_WIFI_SIZE")
        p["wifi_fallback_ssid"] = p["wifi_ssid"]
        self.refused_then_restored(p, "WIFI_DUPLICATE_SSID")

    def test_03_lengths_fail_restore(self):
        for key in ("wifi_ssid", "wifi_fallback_ssid"):
            p = deepcopy(self.dual); p[key] = "x"*33
            self.refused_then_restored(p, "INVALID_WIFI_SIZE")
            p[key] = "x"*32; self.valid(p)
        for key in ("wifi_password", "wifi_fallback_password"):
            for value, error in (("x"*7, "INVALID_WIFI_PASSWORD"), ("z"*64, "INVALID_WIFI_PASSWORD"),
                                 ("x"*65, "INVALID_WIFI_SIZE")):
                p = deepcopy(self.dual); p[key] = value
                self.refused_then_restored(p, error)
            for value in ("", "x"*8, "x"*63, "a"*64):
                p[key] = value; self.valid(p)

    def test_04_ascii_controls_fail_restore(self):
        for key in ("wifi_ssid", "wifi_fallback_ssid", "wifi_password", "wifi_fallback_password"):
            for value in ("bad\nvalue", "bad\x7fvalue", "bad\u00e9value", "bad\0value"):
                p = deepcopy(self.dual); p[key] = value
                expected = "INVALID_PROFILE_FIELD" if "\0" in value and key in self.legacy else "INVALID_WIFI_ASCII"
                self.refused_then_restored(p, expected)

    def test_05_only_one_secondary_and_case_exact(self):
        p = deepcopy(self.dual); p["wifi_third_ssid"] = "SYNTHETIC third"
        self.refused_then_restored(p, "INVALID_PROFILE_KEYS")
        p = deepcopy(self.dual); p["wifi_fallback_ssid"] = self.dual["wifi_ssid"].upper()
        self.valid(p)  # SSID case is meaningful; duplicate check uses exact bytes.


if __name__ == "__main__":
    unittest.main(verbosity=2)
