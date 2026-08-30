# Semantic audit 0080

## Native spell timing APIs

Migrates remaining free spell timing helpers to SpellEntry::GetCastTime(), GetDuration() and IsChanneledSpell(), then removes the obsolete timing wrappers from the compatibility shim.

Core impact: **none**. This patch is Mod-only.
