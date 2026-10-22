---
name: threejs-interaction
description: "Implement or diagnose Three.js raycasting, object picking/hover, camera controls and scene transform/drag interactions for miGarden's 3D representation. Use for pointer, touch or keyboard interaction with a Three.js scene. Not for generic DOM forms, SVG-only events, firmware commands, physical actuation, or selecting a new UI design."
license: MIT
metadata:
  adaptation_version: "mg-h1.1"
  upstream_commit: "0e181866f095b5f06d005133876e926515390d95"
  upstream_api_target: "three.js r186"
---

# Three.js interaction for miGarden

Read the relevant section of [reviewed API examples](references/upstream-r186.md) for raycasting, controls, selection or transforms. They target **r186**. Match the project's actual locked Three.js and renderer versions before using API-specific examples; do not install or upgrade packages to match this reference.

All Agents may select this skill for a matching assigned task without a named invocation or PM/T1 gate. Use the selected product design and actual task grant. Camera navigation and object selection are view interactions; selecting this skill grants no device commands or physical actions.

## Keep representation and intent precise

- Convert pointer coordinates relative to `canvas.getBoundingClientRect()`, including offset and CSS size. Reject zero-sized canvases. Upstream fullscreen world/screen and box-selection examples need adaptation for embedded/resized canvases.
- Pick only the intended objects/layers and resolve hits to stable spatial object IDs shared with SVG. Define whether hidden objects or proxy meshes are selectable; raycasting can include invisible meshes. Preserve local/world transform and source-unit contracts from [fundamentals](../threejs-fundamentals/SKILL.md).
- Coordinate orbit/drag/transform ownership so one gesture does not move both view and object. Provide keyboard/visible selection feedback consistent with the UI brief; pointer events alone do not establish keyboard accessibility.
- A transform preview changes representation. Do not silently overwrite measured source geometry, persist edits, or issue a device command from a pick/drag handler. Follow the actual edit/persistence/command contract if that work is separately authorized.
- Retain named handlers for removal; dispose controls and connected timers on teardown. Preserve shared resources. Verify parent-group, instanced/proxy and empty-hit handling where the actual scene uses them; do not invent a broad interaction matrix.
- Treat CDN/asset loading in examples as optional network behavior. Reuse project packages and approved assets; no browser, npm dependency, provider or runner is installed by this skill.

Use runtime observations for click/hover/drag/resize and actual touch/keyboard support when implementation and testing are in scope. Syntax checks alone prove no interaction behavior; viewport tests do not establish physical-device acceptance. Report absent evidence as `NOT_RUN`.

Source and local changes: [PROVENANCE.json](PROVENANCE.json), [MIT license](LICENSE), [project catalog](../../../docs/skills.md).
