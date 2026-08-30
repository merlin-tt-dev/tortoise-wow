# 0100: Native threat debug spell visual

Replaced absent WorldSession::SendPlaySpellVisual() with a session-local SMSG_PLAY_SPELL_VISUAL packet, preserving master-only debug visibility instead of broadcasting globally.

Ownership: **MOD**.
