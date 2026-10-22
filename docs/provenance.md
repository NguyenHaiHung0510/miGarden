# Design source provenance

The design uses a site survey, preliminary architecture/hardware options and a detailed system specification. The table identifies the original document revisions used on **2026-10-22**. SHA-256 values identify those inputs, not the current public pages or independent validation of their claims.

| Source ID | Technical input | Public document | Original SHA-256 |
|---|---|---|---|
| S01 | Site readings, anchors and environment/RF uncertainty | [Site baseline](site-baseline.md) | `01ce91aea6a9ac083951944782fed14426191cf7550ce6fb215326f6dd7d4df0` |
| S02 | Architecture input | [Architecture](architecture.md) | `c65ee99bf85e2fccaeaecac8d6e18fbfd692aefa04194c94368a206d61b475e5` |
| S03 | Detailed contracts and evidence gates | [Specification](specification.md) | `b86142a7765a254ef51694bf1115b41e91f8f1ae9c7e9f61686311b547804ebf` |
| S04 | Hardware options and constraints | [Project state](project-state.md) | `9935be8bba5c474e72372960b3d2ca380d152e8e76dbe7adfbfbe5f0728879fa` |
| S05 | Requirements and design analysis | [Design inputs](design-inputs.md) | `28c22d8448b963adb9b065f0f455a149bba42ffc7bb0b8bd858ce4fbc3f9db6f` |
| S06 | Site-source inventory and sampled-frame coverage | [Site baseline](site-baseline.md) | `2fc8588b75c30d36d42815072293d8021410a5360f39e3e08cfd802aae12ed30` |
| S07 | SVG/Three.js representation trade-offs | [Representation](representation.md) | `305deac1ec80cf381fa34ba234209c196d1f1e5e1e8f2074dfb72b37b238aa62` |

Current [product decisions](specification.md#20-product-decisions) are folded into the specification: separate system/adapter ownership, dynamic pots and shared Zone A, local schedule and source input, sensing HOLD and A+B. Historical inputs may describe earlier alternatives. Original hashes identify their lineage without making those old alternatives current requirements.

Site sources have not calibrated XYZ/scale or verified radio/hydraulic behavior. Earlier comparison illustrations were synthetic examples, not site reconstructions. [Representation](representation.md#public-content-and-private-sources) defines public content and private-source handling; no measured 3D/COLMAP asset is supplied here.

External skill sources, adaptations, licenses and API-version compatibility are recorded in [skills](skills.md), package `PROVENANCE.json` files and [notices](../THIRD_PARTY_NOTICES.md).
