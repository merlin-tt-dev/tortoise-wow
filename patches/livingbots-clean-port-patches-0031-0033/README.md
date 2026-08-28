# LivingBots Clean Port – Patch Bundle after 0030

Basis / Source of Truth:

- user-provided `tortoise-wow.tar(2).xz`
- snapshot contains Patch 0030 already applied
- expected project branch: `livingbots/playerbots-clean-base`
- last verified remote snapshot corresponding to that source: `ccc380f634a68367bb151b1a61d171b3a0d83c3b`

## Permanent packaging rule

Core and module changes are kept strictly separate:

- `core/` contains only intentional Penqle/Tortoise core extensions plus their maintenance registration/documentation.
- `mod/` contains only `mod-playerbots` changes. No `src/` core modifications are allowed in these patch files.
- `audits/` contains validation records.

This bundle starts *after* the supplied 0030 snapshot. Existing core-0001..core-0004 are already part of that source snapshot and are therefore not duplicated here.

## Apply order

From a pristine copy of the supplied 0030 snapshot:

```bash
git apply mod/0031-build-compatibility-pass-2-native-host-lfg-useitem-randommgr.patch
git apply core/core-0005-player-homebind-location-accessor.patch
git apply mod/0032-build-compatibility-pass-3-rti-targeting.patch
git apply mod/0033-build-compatibility-pass-4-playerbot-factory.patch
```

`core-0005` must be present before compiling the 0032 RTI/targeting state because the module consumes the exact homebind location accessor.

## Integrity

Run from this bundle directory:

```bash
sha256sum -c SHA256SUMS
```

The complete sequence above was independently tested against a freshly extracted copy of `tortoise-wow.tar(2).xz` using:

- `git apply --check`
- apply
- `git diff --check`
- reverse check
- reverse apply
- reapply

The split `core-0005 + mod 0032` tree was also byte-compared with the earlier monolithic 0032 result and is identical.

## Not included yet

0034 and core-0006/core-0007/core-0008 are intentionally **not** included because they have not yet been frozen and fully audited as final patch artifacts.
