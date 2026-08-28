# LivingBots Clean Port — 0034/0035 re-audited

This package supersedes the earlier `livingbots-clean-port-patches-0034-0035.tar.xz`.

The important change is core-0008: the previous stateful corpse-decay-limit patch
has been rejected after re-audit. The replacement only exposes two tiny operations
in `Creature.h`; `Creature.cpp` and Penqle's corpse/loot lifecycle remain untouched.

Directories:
- `core/` — core patches only
- `mod/` — mod-playerbots patches only
- `audits/` — necessity review and apply/reverse/reapply evidence

The repository `patches/` directory is not modified by these patch files. It may be
used as provenance/reference by the maintainer.

See `APPLY_ORDER.txt` before applying.
