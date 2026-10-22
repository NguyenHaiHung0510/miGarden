---
name: threejs-fundamentals
description: "Set up or diagnose Three.js scenes, cameras, renderer resize/lifecycle, scene hierarchy and transforms for miGarden's 3D representation. Use for Three.js-specific scene work or shared SVG/3D coordinate mapping. Not for SVG-only styling, generic React/CSS, COLMAP reconstruction, hardware control, or choosing a new UI design."
license: MIT
metadata:
  adaptation_version: "mg-h1.1"
  upstream_commit: "0e181866f095b5f06d005133876e926515390d95"
  upstream_api_target: "three.js r186"
---

# Three.js fundamentals for miGarden

Use the [reviewed API examples](references/upstream-r186.md) for scene/camera setup, local/world transforms, renderer sizing, timing and disposal. The reference targets **r186**; its import maps use **0.186.0**, while the upstream README reports verification on **0.186.1**. These are source declarations, not an installed dependency or reproduced runtime result.

All Agents may select this skill for a matching assigned task without a named invocation or PM/T1 gate. Follow the current project brief, selected visual direction and actual implementation grant. A scene-planning task remains planning; installing this skill does not authorize product UI implementation.

## Use the project contract

- Inspect the actual package manifest/lockfile and renderer before copying examples. Reconcile version-specific APIs with that version's official documentation/source. Do not upgrade Three.js or switch WebGL/WebGPU, React Three Fiber or asset tooling merely to match the reference. No library or provider is bundled here.
- SVG and Three.js consume the same versioned spatial input. Preserve object IDs, units, origin/axes, scale, measurement source and uncertainty. Define the explicit source-to-scene and SVG-to-scene transforms; Three.js defaults do not establish the site's axes or dimensions.
- Keep measured, inferred and unknown geometry distinguishable. Visual plausibility or a screenshot does not prove site measurements, RF coverage, hydraulics or device state.
- Adapt fullscreen examples to the actual container: use its CSS dimensions, update the appropriate camera projection on resize, and dispose only resources owned by the view. Release event handlers, animation loop and connected timer on teardown; preserve shared geometry/material/texture ownership.
- Prefer existing project packages and approved local assets. CDN import maps are alternatives shown by upstream; evaluating or copying them does not grant network asset downloads, dependency installation or upload of private raw captures. Do not log private asset URLs or credentials.

Use [threejs-interaction](../threejs-interaction/SKILL.md) when picking or camera controls are part of the task. Other upstream modules mentioned in the reference are not installed dependencies. Reuse existing UI/accessibility and browser verification workflows when those tasks need them.

Report the actual package/renderer and what was checked. Parsing examples proves syntax only; runtime rendering, GPU/device support, accessibility and measurement correctness need their own evidence and stay `NOT_RUN` until observed.

Source and local changes: [PROVENANCE.json](PROVENANCE.json), [MIT license](LICENSE), [project catalog](../../../docs/skills.md).
