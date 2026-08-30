# Final Playerbot C++ Compile/API Audit

Baseline: remote source state `1f2db9e808d7120a0a6ecf49f3df610c579824b1`.

The configured ZERO/Turtle build exposes 458 C/C++ translation units under `modules/mod-playerbots`:
- 412 strategy TUs
  - 14 strategy root
  - 143 actions
  - 56 generic
  - 84 class-specific
  - 13 tests
  - 19 triggers
  - 83 values
- 37 top-level playerbot TUs
- 7 AHBot TUs
- `ModuleLoader.cpp`
- `botpch.cpp`

All configured Playerbot C/C++ source files were covered by the compile/API sweep. There are no additional C/C++ source files under `modules/mod-playerbots` outside `compile_commands.json`.

After 0105, all covered active ZERO translation units are syntax-clean. Every TU directly changed by 0101–0105 also passed an O0/g0 object compile.

Residual compatibility scan:
- no `cmangos-compat*` references remain;
- known WotLK Dungeon Finder APIs remain only in dormant `MANGOSBOT_TWO` code;
- an old `GetGraveyardManager()` occurrence remains only inside commented `MANGOSBOT_ONE` code;
- active ZERO LFG remains based on native Turtle/Penqle `sLFGMgr` Meeting-Stone behavior.

A full `ninja modules` run was intentionally not used as the completion criterion because the target first pulls a large Core/dependency build graph. The partial attempt produced no compiler diagnostics before being stopped. Full repository build/link remains an end-to-end build validation step, distinct from this Playerbot compile/API sweep.

Post-shim-removal confidence check:
- the Actions directory had already been fully swept before 0091;
- after 0091, all removed shim symbols were audited module-wide;
- Actions that actually exercise the removed helper/include surface were recompiled on the final shim-free source, including ChooseTravelTargetAction, SayAction, RpgSubActions, LfgActions, GuildShareAhBuyAction, UseItemAction, ListSpellsAction, GenericActions, GuildCraftOrderAction, LootAction, SetCraftAction, and DebugAction; all passed syntax compile;
- an additional first alphabetic Action batch also recompiled clean before the redundant full re-sweep was stopped for CPU cost.

This closes the remaining risk that 0091 could have broken an Action TU that had only been compiled before shim removal.
