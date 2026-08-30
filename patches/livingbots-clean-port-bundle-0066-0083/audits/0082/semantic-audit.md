# Semantic audit 0082

## Native WHO logout state

Uses WorldSession::isLogingOut() rather than conflating logout with a generic Unit stun state.

Core impact: **none**. This patch is Mod-only.
