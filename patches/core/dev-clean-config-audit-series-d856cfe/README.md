# dev-clean Config Audit – Patch Bundle

Base branch: `dev-clean`  
Required starting point: `d856cfe89b9dba0a0c147bf5a38018df2cbd1cca`

This archive contains the config-audit series that comes **after** the already committed
`core-buildsystem-003` split-generator work. It intentionally does not contain earlier
012/013/003 patches because they are part of the required base state.

## Apply order

Apply exactly in this order:

1. `01-core-buildsystem-004-finalize-config-dist-layout.patch`
   - renames the old generated monolith to `mangosd-legacy.conf.dist`
   - makes `mangosd.conf.dist` the new IncludeFile root
   - generates fragments below `mangosd.conf.d.dist/`
   - generated root references runtime path `mangosd.conf.d/...`

2. `02-core-only-014-config-dist-audit-cleanup.patch`
   - completes the mangosd config content audit
   - removes dead/obsolete config keys and legacy unused config templates
   - fixes config-name drift
   - adds currently active Core keys missing from the distribution config
   - removes duplicate mangosd keys

3. `03-core-only-015-anticheat-config-audit-fixes.patch`
   - fixes active anticheat config defects
   - loads BanWave.Day/Hour/Minute
   - fixes BanWave minute getter
   - fixes duplicated Warden.TotalCount / missing Warden.TotalAction entry

4. `04-core-buildsystem-005-package-anticheat-config-dist.patch`
   - generates/packages `anticheat.conf.dist` as a separate active config family

## Manual apply

From the tortoise-wow repository root:

```bash
for p in \
  /path/to/bundle/patches/01-core-buildsystem-004-finalize-config-dist-layout.patch \
  /path/to/bundle/patches/02-core-only-014-config-dist-audit-cleanup.patch \
  /path/to/bundle/patches/03-core-only-015-anticheat-config-audit-fixes.patch \
  /path/to/bundle/patches/04-core-buildsystem-005-package-anticheat-config-dist.patch
do
  git apply --check -vv "$p" || exit 1
  git apply -vv "$p" || exit 1
  git diff --check || exit 1
done
```

Or use:

```bash
./scripts/apply-series.sh /path/to/tortoise-wow
```

The helper does **not** commit or push anything.

## Rollback order

Patch rollback must happen in the exact reverse dependency order:

1. `04-core-buildsystem-005-package-anticheat-config-dist.patch`
2. `03-core-only-015-anticheat-config-audit-fixes.patch`
3. `02-core-only-014-config-dist-audit-cleanup.patch`
4. `01-core-buildsystem-004-finalize-config-dist-layout.patch`

Manual rollback:

```bash
for p in \
  /path/to/bundle/patches/04-core-buildsystem-005-package-anticheat-config-dist.patch \
  /path/to/bundle/patches/03-core-only-015-anticheat-config-audit-fixes.patch \
  /path/to/bundle/patches/02-core-only-014-config-dist-audit-cleanup.patch \
  /path/to/bundle/patches/01-core-buildsystem-004-finalize-config-dist-layout.patch
do
  git apply -R --check -vv "$p" || exit 1
  git apply -R -vv "$p" || exit 1
  git diff --check || exit 1
done
```

Or use:

```bash
./scripts/rollback-series.sh /path/to/tortoise-wow
```

### Important rollback note

If these patches have already been committed and later commits modify the same files,
prefer `git revert <commit>` in reverse commit order. Reverse-applying historical patch
files is safest only while the touched files still match the state produced by this series.
Never use `git reset --hard` on a shared/pushed branch merely to remove this series.

## Expected build output after the full series

```text
src/mangosd/
├── mangosd
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

Runtime deployment convention:

```bash
cp mangosd.conf.dist mangosd.conf
cp -a mangosd.conf.d.dist mangosd.conf.d
cp anticheat.conf.dist anticheat.conf
```

See `docs/dev-clean-config-audit-final.md` for the audit findings and rationale.
