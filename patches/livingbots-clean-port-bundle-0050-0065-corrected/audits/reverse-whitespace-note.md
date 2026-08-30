# Reverse whitespace note

The complete reverse test (0065 → 0050) can print whitespace warnings when it restores lines that already contained whitespace issues in the pre-patch baseline (notably around 0059, and some baseline tab formatting restored by earlier patches).

This is not newly introduced whitespace: every forward patch has a clean `git diff --check`, and the complete reapply returns to the expected 64/64 byte-identical final state.
