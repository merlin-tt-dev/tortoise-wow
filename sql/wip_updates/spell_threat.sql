-- Spell 25918's `spell_threat` row carries the exact same threat, multiplier
-- and ap_bonus as its rank 1, so the engine's own consistency check
-- (DoSpellThreat::operator() in SpellMgr.cpp) already flags it as duplicate
-- data it would have filled in from rank 1 regardless:
--
--     Spell 25918 listed in `spell_threat` as custom rank has same data as
--     Rank 1, so redundant
DELETE FROM `spell_threat` WHERE `entry` = 25918;
