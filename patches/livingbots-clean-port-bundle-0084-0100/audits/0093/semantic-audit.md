# 0093: Native current spell effect damage

Replaced the absent upstream Spell::GetDamage() accessor with Penqle Spell::damage, preserving the per-effect value semantics rather than changing to total spell damage.

Ownership: **MOD**.
