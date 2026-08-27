#pragma once

#define CAST_ANGLE_IN_FRONT (2 * M_PI_F / 3)
#define EMOTE_ANGLE_IN_FRONT (2 * M_PI_F / 6)

// Dynamic Detour area ids used to increase path cost around hostile units.
constexpr uint8 PLAYERBOT_MMAP_AREA_AVOID = 12;
constexpr uint8 PLAYERBOT_MMAP_AREA_DANGER = 13;
