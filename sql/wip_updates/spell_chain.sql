-- A clean boot logs complaints about `spell_chain` rows that were traced
-- through SpellMgr::LoadSpellChains to find out what actually happens to the
-- flagged row at runtime, not just what the warning says.

-- 26 dead rows (delete, zero behavioral change):
-- 19 are byte-for-byte duplicates of what the engine already derives on its
-- own from Talent.dbc / skill_line_ability. The loader detects the match and
-- discards the row unused (chain_itr found, rank/prev/first match, req == 0 ->
-- "already added ... and non need in `spell_chain`").
-- 7 are custom cross-links that never take effect because the real chain data
-- wins over them, or the row is already self-flagged redundant:
--   - 24858 (Moonkin Form) and 45734 (Owlkin Frenzy) claim to be ranks of each
--     other; they are unrelated spells. skill_line_ability shows Moonkin
--     Form's real successor is 51430. Both rows are rejected on load and the
--     correct auto-derived entry underneath is untouched.
--   - 45599/45560/45960 graft an unrelated talent (45560, "Recurring Shield")
--     onto the Crush rank line as a fake rank 2. 45599 is structurally
--     invalid on its own (rank 1 with a non-zero prev_spell); the real Crush
--     chain (1464 -> 8820 -> 11605 -> 45961) is built automatically and needs
--     none of this.
--   - 45910/45911 share a name ("Mana Funnel" rank 1/2) but are not linked:
--     skill_line_ability shows 45911's real successor is spell 1941. 45911's
--     row is rejected and its correct entry is untouched; 45910 loads but the
--     engine already flags it "single rank data, so redundant".
DELETE FROM `spell_chain` WHERE `spell_id` IN (
    13165, 14318, 14319, 14320, 14321, 14322, 25296,
    20043, 20190,
    51346, 51565, 51566,
    51433, 51434, 51435,
    52714, 52715, 52716, 52717,
    24858, 45734,
    45599, 45560, 45960,
    45910, 45911
);

-- Spell 3035 ("Steady Shot" rank 1) is the only rank-1 row in the table with
-- first_spell = 0 instead of first_spell = spell_id - every other rank-1 row
-- follows the (id, 0, id, 1, 0) pattern. Spell 0 does not exist, so the row is
-- rejected, which cascades into rank 2 (3036) failing its "previous rank"
-- lookup. One-value fix resolves both.
UPDATE `spell_chain` SET `first_spell` = 3035
WHERE `spell_id` = 3035 AND `first_spell` = 0;

-- Talent 16689 (Nature's Grasp rank 1) carries req_spell = 339 (Entangling
-- Roots rank 1), conflicting with Talent.dbc's own DependsOnSpell = 0 for this
-- talent. The mismatch gets the row rejected, which cascades into rank 2
-- (16810) failing its "previous rank" lookup. req_spell is read in exactly
-- one place at runtime (the trainer spell list in NPCHandler.cpp), and
-- Nature's Grasp is talent-point only - 0 rows in npc_trainer for any of its
-- 6 ranks - so the value has no live effect either way. Its tooltip always
-- triggers "Entangling Roots (Rank 1)" regardless of known rank, so there was
-- never a rank-matched requirement behind this value. Clearing it to match
-- Talent.dbc restores normal spellbook rank replacement when a player trains
-- rank 2 and up.
UPDATE `spell_chain` SET `req_spell` = 0
WHERE `spell_id` = 16689 AND `req_spell` = 339;
