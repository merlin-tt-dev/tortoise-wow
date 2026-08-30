# Bundle validation 0084-0100

Baseline source state: remote `1534319ee2ecead76482035f7c15be3e3cd5e5ad`, reconstructed locally as audit baseline commit `6917687` for source-only validation.

Final audit source state: local audit commit `ef97496` (0100).

## Results

- Physical patches: **18** (17 Mod + 1 Core).
- Fresh forward `git apply --check`: **18/18 PASS**.
- Fresh forward `git apply`: **18/18 PASS**.
- `git diff --check` after fresh apply: **PASS**.
- Changed source paths compared byte-for-byte with the frozen 0100 audit tree: **45/45 identical**.
- Reverse order `git apply --reverse --check`: **18/18 PASS**.
- Reverse order `git apply --reverse`: **18/18 PASS**.
- After reverse: working tree identical to the reconstructed 1534319 source baseline; `git diff` empty.
- Reapply forward `git apply --check`: **18/18 PASS**.
- Reapply forward `git apply`: **18/18 PASS**.
- Reapply byte comparison: **45/45 identical** to the frozen 0100 audit tree.
- Patch ownership: **PASS**. Every Mod patch touches only `modules/mod-playerbots/...`; `core-0012` touches only Core `src/...`.

The local baseline commit is synthetic because the audit runtime reconstructs source from the archived 35eaa0c tree plus the already-applied 0066-0083 source patches. Repository bookkeeping under `patches/` is not part of that synthetic tree; the source paths consumed by this bundle match the verified 1534319 patch history.
