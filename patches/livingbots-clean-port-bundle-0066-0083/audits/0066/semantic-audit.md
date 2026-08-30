# Semantic audit 0066

## Native talent-point calculation

Replaces the private Player::CalculateTalentsPoints() dependency with Penqle's equivalent public-state calculation (level points + bonus talent count, scaled by Rate.Talent). No Core API is opened.

Core impact: **none**. This patch is Mod-only.
