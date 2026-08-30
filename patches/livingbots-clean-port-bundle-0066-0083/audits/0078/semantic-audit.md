# Semantic audit 0078

## Native SendMail whisper packets

Replaces five removed Player::Whisper calls with native ChatHandler packet/session delivery; mail-send behavior is otherwise unchanged.

Core impact: **none**. This patch is Mod-only.
