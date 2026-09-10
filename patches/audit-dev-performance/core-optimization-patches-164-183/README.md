# Core Optimization Patches 164–183

Target branch: `audits/dev-performance`

Baseline: the complete user-supplied `tortoise-wow-audits-dev-performance-patch163.tar.xz`, verified before work began. No GitHub/source-chunk reconstruction was used.

This bundle continues the performance/deep-modernization audit after patch 163. The proprietary game client is treated as a hard compatibility boundary: no patch is intended to change packet bytes, wire layout, gameplay ordering, RNG-to-target mapping, visibility decisions, chat payloads, or completion/timing semantics.

## Contents

- `patches/core/` — patches 164 through 183
- `APPLY-ORDER.txt` — strict numeric apply order
- `SEMANTIC-AUDIT.md` — per-patch semantic/lifetime/ordering review
- `VALIDATION.txt` — validation performed
- `SHA256SUMS` — checksums of bundle contents

No automatic apply script is included.

## Integration status

The complete 20-patch sequence was applied to a fresh copy of the exact patch-163 archive. Every patch passed strict `git apply --check`, `--numstat`, actual sequential apply, `git diff --check`, added-tab-byte scan, and conflict-marker scan. The resulting tree matched the separately audited working tree byte-for-byte.

Full Linux/MinGW builds are intentionally left to the user's build server.
