# Design source provenance

## Current public baseline

**2026-11-03 · Asia/Saigon.** [Design 01–04](design/README.md) is the only editable current product-design source: purpose/requirements → five processes → eight domain concepts → information semantics. Earlier private working snapshots remain history, not another canonical edition. Publication-baseline acceptance is separate from code, independent review, device or water acceptance. Steps 05–10 remain pending.

The method follows IoT Design and Development, Chapter 4 (requirements, process, domain and information steps). The course is a method reference, not automatic authority for sensors, infrastructure or product scope. No raw slide deck or private discussions are distributed.

[Walkthrough W00–W06](design/walkthrough/README.vi.md) uses synthetic examples from information model 0.2. Its `prepare_data.py` defines data/derived views; `validate_data.py` checks selected semantic constraints. Finished figures are curated existing outputs from the same content, with their embedded attribution retained. Static HTML/text and public-safe lineage replace private review links; no raw QA receipts are published. See the walkthrough guide for exact provenance and reproduction limits.

## Historical input lineage — reference only

The table identifies original input revisions used on **2026-10-22**. Hashes identify those historical inputs, not current public pages or independent validation. Legacy specification/architecture/design-inputs retain their old paths; internal status/requirements/numeric values are historical. They do not define the current domain, API or implementation. Reuse of an unselected idea requires rationale and an explicit choice in the appropriate new design step.

| Source ID | Technical input | Public document | Original SHA-256 |
|---|---|---|---|
| S01 | Site readings, anchors and environment/RF uncertainty | [Site baseline](site-baseline.md) | `01ce91aea6a9ac083951944782fed14426191cf7550ce6fb215326f6dd7d4df0` |
| S02 | Architecture input | [Architecture](architecture.md) | `c65ee99bf85e2fccaeaecac8d6e18fbfd692aefa04194c94368a206d61b475e5` |
| S03 | Detailed contracts and evidence gates | [Specification](specification.md) | `b86142a7765a254ef51694bf1115b41e91f8f1ae9c7e9f61686311b547804ebf` |
| S04 | Hardware options and constraints | [Project state](project-state.md) | `9935be8bba5c474e72372960b3d2ca380d152e8e76dbe7adfbfbe5f0728879fa` |
| S05 | Requirements and design analysis | [Design inputs](design-inputs.md) | `28c22d8448b963adb9b065f0f455a149bba42ffc7bb0b8bd858ce4fbc3f9db6f` |
| S06 | Site-source inventory and sampled-frame coverage | [Site baseline](site-baseline.md) | `2fc8588b75c30d36d42815072293d8021410a5360f39e3e08cfd802aae12ed30` |
| S07 | SVG/Three.js representation trade-offs | [Representation](representation.md) | `305deac1ec80cf381fa34ba234209c196d1f1e5e1e8f2074dfb72b37b238aa62` |

Selected principles already carried into the new baseline include local autonomy, honest desired/applied/reported/physical distinctions, safety/accounting and separate system/adapter ownership. Dynamic managed-pot requirements, notification lifecycle, old schema/transport and numeric thresholds have not been carried forward automatically. Proposed SAF-P01 values are recorded at the new index for future Step 08 decision, not approved policy.

The spatial source remains `viewer/model.json`. Site sources have not calibrated all XYZ/scale or verified RF/hydraulics. Finished site drawings may be public under [content policy](representation.md#public-content-and-private-sources); raw captures, private identifiers and source transcripts stay private. Walkthrough illustrations are synthetic, not site reconstructions. No measured 3D/COLMAP reconstruction is supplied here.

External skill sources, adaptations, licenses and API-version compatibility remain recorded in [skills](skills.md), package `PROVENANCE.json` files and [notices](../THIRD_PARTY_NOTICES.md). Site/model sources and third-party license files are preserved by this transition.
