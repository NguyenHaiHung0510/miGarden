# Project skills — miGarden

Three project-local Markdown skill packages are installed under `.agents/skills/`. Every Agent may select a suitable skill automatically for its assigned task; named invocation is optional. Read only the matching entry and references needed for the work. Each package sets `allow_implicit_invocation: true` in `agents/openai.yaml`.

| Skill and purpose | Immutable source and license | Version and compatibility |
|---|---|---|
| [esp32-development](../.agents/skills/esp32-development/SKILL.md): board/pin/power, bring-up planning, boot diagnosis and build-to-device evidence | [magnus919/agent-skills at `92c327e525da69c9ad3812fd223a1770e977c2e3`](https://github.com/magnus919/agent-skills/tree/92c327e525da69c9ad3812fd223a1770e977c2e3/esp32-development), [MIT](../.agents/skills/esp32-development/LICENSE) | Adaptation `mg-h1.1`; no upstream skill semantic version declared in the reviewed entry. Includes three engineering references, three worksheets and Windows preflight. Excludes executable helper, firmware/config templates and framework installation. An empty unsupported COM enumeration is not board-absence evidence. |
| [threejs-fundamentals](../.agents/skills/threejs-fundamentals/SKILL.md): scene/camera/renderer lifecycle and transforms | [CloudAI-X/threejs-skills at `0e181866f095b5f06d005133876e926515390d95`](https://github.com/CloudAI-X/threejs-skills/tree/0e181866f095b5f06d005133876e926515390d95/skills/threejs-fundamentals), [MIT](../.agents/skills/threejs-fundamentals/LICENSE) | Adaptation `mg-h1.1`; reference API target r186, examples use `three@0.186.0`, upstream README reports testing on `0.186.1`. Adds shared SVG/3D units, coordinates and provenance. These declarations do not pin the future product dependency. |
| [threejs-interaction](../.agents/skills/threejs-interaction/SKILL.md): picking, hover, camera, drag and transform | [CloudAI-X/threejs-skills at the same revision](https://github.com/CloudAI-X/threejs-skills/tree/0e181866f095b5f06d005133876e926515390d95/skills/threejs-interaction), [MIT](../.agents/skills/threejs-interaction/LICENSE) | Adaptation `mg-h1.1`, same API target. Adds canvas-relative coordinates, stable IDs, view/edit/command separation, cleanup and keyboard evidence. No React Three Fiber dependency or uninstalled full-suite links. |

Each package's `PROVENANCE.json` records source SHA/path, source-file and license SHA-256/Git blob, local modifications, dispositions and installed payload hashes. Preserve attribution and [notices](../THIRD_PARTY_NOTICES.md) when updating; review source/version changes without overwriting local adaptations. MIT terms here do not establish a license for unrelated miGarden material.

## Choose by task

| Task | Matching skill |
|---|---|
| Plan ESP32 bring-up from schematic, diagnose boot/brownout, assess pins/power | ESP32 development |
| Map one spatial fixture to SVG/Three.js, fix transforms or camera resize | Three.js fundamentals |
| Pick a pot in an embedded canvas, synchronize IDs, resolve control/drag gestures | Three.js interaction |
| Edit prose or pure SVG styling, validate a login form, reconstruct with COLMAP | None of these solely for that task |

Use other available skills when relevant; this catalog does not require installing a personal workflow or an entire upstream suite. Picking a mesh does not authorize an OPEN command. Skill selection does not itself authorize toolchain install, serial/reset/flash, provisioning or water operation; follow [AGENTS](../AGENTS.md) and the relevant [runbook](operations.md).

## Dependencies and limits

Reading requires Markdown/YAML support. Packages contain no executable helper, hook, plugin/MCP server or credential requirement. JavaScript examples are references and do not execute when an entry is read. Future firmware work needs the selected board/SDK/toolchain and resource identity. Future Three.js work needs the product's chosen version/lockfile and renderer/browser/GPU evidence; reference CDN import maps are not a product deployment decision.

Codex discovery/automatic selection was observed for one fresh-root planning task covering ESP32 and a 3D viewer on 2026-10-22. This establishes a limited discovery example, not an always-trigger guarantee. Other runtime discovery is unverified. Rendering/interaction, build/firmware/device and field workflows have not been run. Recheck guidance when upstream APIs, selected SDK/Three.js versions or real use expose a mismatch.
