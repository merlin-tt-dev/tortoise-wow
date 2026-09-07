# dev-clean Core Config Audit – final

Basis: `dev-clean` at `d856cfe89b9dba0a0c147bf5a38018df2cbd1cca` after core-buildsystem-003.

## Final target layout

The build should publish:

```text
src/mangosd build directory/
├── mangosd-legacy.conf.dist
├── mangosd.conf.dist
├── mangosd.conf.d.dist/
│   ├── 10-core-runtime.conf
│   ├── 20-database-storage.conf
│   ├── 30-network-login.conf
│   ├── 40-logging-monitoring.conf
│   ├── 50-world-gameplay.conf
│   ├── 60-rates-economy.conf
│   ├── 70-pvp-battlegrounds.conf
│   ├── 80-maps-visibility.conf
│   ├── 90-security.conf
│   ├── 95-services-custom.conf
│   ├── MANIFEST.txt
│   └── README.txt
└── anticheat.conf.dist
```

`mangosd.conf.dist` references runtime paths named `mangosd.conf.d/...`, not `.dist` paths. Deployment therefore consists of copying `mangosd.conf.dist` to `mangosd.conf` and copying/renaming `mangosd.conf.d.dist/` to `mangosd.conf.d/`.

`mangosd-legacy.conf.dist` remains the complete monolithic compatibility template.

## Historical mangosd.conf.dist.in audit

Original state:

- 624 assignments
- 621 unique keys
- 3 identical duplicate assignments

Final proposed state after core-only-014:

- 569 assignments
- 569 unique keys
- 0 duplicate assignments
- split fragments contain exactly the same 569 non-control assignments as the cleaned source

### Removed as dead/misleading

- 108 legacy `Anticheat.*` assignments. The compiled Anticheat implementation uses a separate `AnticheatConfig` instance backed by `anticheat.conf`; the old `Anticheat.*` mangosd block has no active consumer.
- 10 obsolete global Warden assignments. The old `src/game/Anticheat/WardenAnticheat/` implementation that used most of them is not in the game CMake source list. `Warden.ModuleDir` is also owned by the active separate AnticheatConfig. The two global settings still consumed by compiled code remain: `Warden.WinEnabled` and `Warden.DebugLog`.
- `LoginVIPQueue` and `LoginVIPQueueLevelThreshold`: no active loader/consumer; the only setter reference for the threshold is commented out.
- `Progression.UnlinkedAuctionHouses`: enum exists but no active setter/consumer.
- `ShowProgressBars`: no active consumer.
- `MaxCreaturesStealthDetectRange`: no active consumer; current Core only uses `MaxPlayersStealthDetectRange`.
- `Antiflood.Sanction`: its World.cpp setter is commented out.
- `Visibility.Distance.Continents.Min`: no active consumer.
- `BeginnersGuilds`, `BeginnersGuildHorde`, `BeginnersGuildAlliance`: no active consumer.
- Second repeated AV/WS/AB battleground honor-rate block.

### Corrected key-name drift

- `GmLogFile` -> `GMLogFile`
- `vmap.petLOS` -> `vmap.petLoS`
- `OutdoorPvp.EPEnabled` -> `OutdoorPvP.EP.Enable`
- `OutdoorPvp.SIEnabled` -> `OutdoorPvP.SI.Enable`

These new names match the actual current Core reads.

### Added missing current-Core settings

76 current-Core config keys absent from the historical dist are added with values matching current runtime defaults. They cover account/session policy, analysis, the main-Core antispam implementation, battleground/PvP, dynamic respawn/scaling/visibility, queue policy, item/restore logging, Smartlog/export, and HTTP/API/translation settings.

The separate AnticheatConf antispam settings remain separate; identical `Antispam.*` prefixes in two config files belong to different Config instances.

### Remaining apparently unreferenced template keys

After cleanup the only keys without a literal exact source occurrence are 11 database pool keys:

- `LoginDatabase.{Connections,WorkerThreads}`
- `WorldDatabase.{Info,Connections,WorkerThreads}`
- `CharacterDatabase.{Info,Connections,WorkerThreads}`
- `LogsDatabase.{Info,Connections,WorkerThreads}`

These are valid. `src/mangosd/Master.cpp` constructs them dynamically from the database name (`name + "Database.Info"`, etc.). They must remain.

## rate.conf.dist.in

Verdict: orphan/dead template; remove.

- `_RATE_CONFIG` exists only as an unused path define.
- There is no source call that loads `rate.conf`.
- The section/profile helpers have no current caller using the rate template.
- `Rate.XP.Kill` is already a normal current mangosd setting; the level-profile mapping in `rate.conf.dist.in` has no runtime consumer.

No migration into `mangosd.conf.d` is appropriate because doing so would imply profile semantics that the current Core does not implement.

## mods.conf.dist.in

Verdict: orphan/dead template; remove.

- `_MODS_CONFIG` exists only as an unused path define.
- There is no loader for `mods.conf`.
- `Mod.Tbc.DiminishingDuration` occurs only in the template and has no runtime consumer.

No migration into `mangosd.conf.d` is appropriate.

## anticheat.conf

Verdict: active and meaningful, but intentionally separate from mangosd.conf.

Evidence:

- `Anticheat::AnticheatConfig` is its own `Config` subclass.
- current Core calls `SetSource("anticheat.conf")` / `_LIB_ANTICHEAT_CONFIG` and `loadConfigSettings()`.
- therefore `[AnticheatConf]` must not be merged into `[MangosdConf]` fragments.

The existing safe/read-only profile should instead be emitted and installed as `anticheat.conf.dist`.

### Real Anticheat config bugs found

1. `BanWave.Day`, `BanWave.Hour`, `BanWave.Minute` exist in the template and enum/getter, but were never loaded into the AnticheatConfig arrays.
2. `GetBanWaveTime()` returned `CONFIG_UINT32_AC_BAN_WAVE_DAY` for `minute` instead of `CONFIG_UINT32_AC_BAN_WAVE_MINUTE`.
3. The read-only template declared `Warden.TotalCount` twice; the second entry must be `Warden.TotalAction`.

core-only-015 repairs all three without changing any currently used ban-wave behavior (there is no current caller of `GetBanWaveTime()`), while making the existing config API internally correct.

## Proposed patch series

1. `core-buildsystem-004-finalize-config-dist-layout.patch`
   - promote split root to `mangosd.conf.dist`
   - rename monolith to `mangosd-legacy.conf.dist`
   - emit fragments as `mangosd.conf.d.dist/`
   - root references `mangosd.conf.d/...`
   - install root, legacy file, and dist directory
   - rename `90-security-anticheat.conf` to `90-security.conf`

2. `core-only-014-config-dist-audit-cleanup.patch`
   - remove dead historical mangos/Warden/Anticheat settings
   - fix key-name drift
   - remove three duplicate BG keys
   - add 76 missing current-Core defaults
   - remove dead `rate.conf.dist.in`, `mods.conf.dist.in`, `_RATE_CONFIG`, `_MODS_CONFIG`

3. `core-only-015-anticheat-config-audit-fixes.patch`
   - load BanWave values
   - fix BanWave minute getter
   - fix `Warden.TotalAction` typo in read-only profile

4. `core-buildsystem-005-package-anticheat-config-dist.patch`
   - generate/install `anticheat.conf.dist` from the read-only profile
   - deliberately keep AnticheatConfig outside `mangosd.conf.d`

## Validation performed

Starting from the d856cfe content snapshot, all four patches were applied sequentially with `git apply --check` and `git diff --check` passing after every patch.

The final split generator was executed with CMake script mode:

- source assignments: 569
- generated fragment non-control assignments: 569
- duplicate mangosd assignments: 0
- duplicate anticheat template assignments: 0
- generated root references `mangosd.conf.d/...`
- generated dist directory is `mangosd.conf.d.dist/`
