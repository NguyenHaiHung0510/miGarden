---
name: esp32-development
description: "Plan ESP32 bring-up, inspect board/pin/power facts, diagnose firmware or boot logs, and define build-to-device evidence for miGarden. Use for ESP32-specific engineering within the assigned task. Not for generic web/Three.js work, non-ESP32 boards, or a general security audit. Selecting this skill does not authorize hardware operations."
license: MIT
metadata:
  adaptation_version: "mg-h1.1"
  upstream_commit: "92c327e525da69c9ad3812fd223a1770e977c2e3"
---

# ESP32 development for miGarden

Support the current project stack and task. Establish exact board/SoC/module/revision, schematic, USB transport, flash/PSRAM, power source, attached components and recovery artifacts before selecting pins or producing executable device instructions. Keep absent evidence `UNKNOWN`; a USB bridge label or COM number does not establish chip or running firmware identity.

This package supplies Markdown procedures and worksheets. It installs no toolchain, board detector, firmware runner or Python helper. All Agents may select it when relevant within their assignment; no named invocation or PM/T1 gate is required. Selection does not expand the task's permitted effects.

## Apply only the needed path

| Task | Read |
|---|---|
| Windows host/COM mapping and command effects | [Windows preflight](references/windows-preflight.md) |
| Pin, voltage, supply, driver or load decisions | [Hardware and electrical safety](references/hardware-and-electrical-safety.md) and the exact device documents |
| Boot, serial, power, crash or recovery diagnosis | [Debugging and recovery](references/debugging-recovery-and-security.md) |
| Plan target/bench tests and independent observations | [Hardware test automation](references/hardware-test-automation.md) |

Use [bring-up](templates/hardware-bringup.md), [component contract](templates/component-contract.md) or [hardware test plan](templates/hardware-test-plan.md) only when that record is useful; reuse existing project artifacts rather than creating a second authoritative ledger. Worksheet fields and upstream recovery procedures are planning aids, not instructions to carry out every step.

## Engineering and evidence

- Reuse the framework and versions already selected by the project. If unresolved, compare bounded options and leave the selection open; do not install ESP-IDF, PlatformIO or another framework merely because upstream lists it.
- Resolve pins and electrical limits from the exact schematic/datasheets. Do not transfer family pin maps, invent safety thresholds, disable brownout protection, or drive a pump/relay/solenoid directly from GPIO. Verify inactive states through reset and boot for the actual circuit.
- For a build task that is separately authorized, inspect build scripts for network, provisioning and target side effects first. Bind source/dirty state, tool versions, sdkconfig, partitions, generated flash arguments, ELF/map and image digests. A successful build proves compilation only.
- Distinguish transfer, booted image, simulated behavior, dry LED behavior, contained-water behavior and field behavior. Use independently observable evidence for the claimed plane; a firmware log or UI indicator alone cannot prove valve or water behavior.
- Board/COM/fixture ownership must match the task before exclusive access. Native test fixtures may flash/reset during setup. Opening serial, querying chip identity, or monitoring may alter reset lines or target state. A timeout can leave effects: inspect state before retrying.
- Actual flash/reset/erase, power cycling, pin/bus probes, OTA, credential/provisioning changes and physical loads require the corresponding task authorization, exact target, expected effects and recovery path. Existing authorization may cover the sequence; skill use is not a reason for routine reapproval. Stop the affected action when identity, ownership or safe state is unresolved.
- Secure boot, flash encryption, eFuses and download restrictions are lifecycle decisions with potentially irreversible effects. Keep general security assessments with the existing security-review workflow when available; do not turn routine firmware diagnosis into a broad audit or execute security provisioning from a reference.

For version-sensitive commands consult the selected SDK's official documentation and installed help. Help proves syntax, not safe operating conditions. Keep private firmware/config/serial contents and credentials within the task's data boundary.

Return the narrow conclusion, source/evidence and remaining unknowns. Mark unrun build/device/field steps `NOT_RUN`; do not require physical testing to answer a bounded planning or log-analysis request.

Source and local changes: [PROVENANCE.json](PROVENANCE.json), [MIT license](LICENSE), [project catalog](../../../docs/skills.md).
