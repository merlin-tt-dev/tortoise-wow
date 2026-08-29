# LivingBots Clean Port Bundle 0040-0041

Standardisiertes Patch-Bundle für `livingbots/playerbots-clean-base`.

## Festes Archivschema ab jetzt

- `patches/` — alle anwendbaren `.patch`-Dateien, unabhängig davon ob Core/Mod/Compatibility
- `audits/<patchnummer>/` — alle Audit-, Build- und Freeze-Nachweise des jeweiligen Patches
- `APPLY_ORDER.txt` — verbindliche Apply-Reihenfolge
- `MANIFEST.txt` — vollständige Dateiliste
- `SHA256SUMS` — Prüfsummen aller Dateien im Bundle (außer SHA256SUMS selbst)

Keine wechselnden `mod/`, `core/` oder Root-Patch-Lagen mehr.

## Apply

Aus dem Repository-Root, nachdem die Dateien aus `patches/` in den lokalen `patches/`-Ordner kopiert wurden:

```bash
git apply --check patches/0040-build-compatibility-pass-11-native-gossip-chat-worldlocation.patch
git apply patches/0040-build-compatibility-pass-11-native-gossip-chat-worldlocation.patch

git apply --check patches/0041-build-compatibility-pass-12-native-trainer-rpg.patch
git apply patches/0041-build-compatibility-pass-12-native-trainer-rpg.patch
```

0040 und 0041 wurden in dieser Reihenfolge gemeinsam getestet.
