-- Example content for mod-worldoverlay.
--
-- This file is intentionally outside data/sql/world and is NOT an automatic
-- migration. It documents the existing Tele City proof-of-concept target.
--
-- Tele City uses the hidden constructed area on map 36. The normal Deadmines
-- entrance/exit AreaTriggers and ordinary dungeon instance path remain untouched.

INSERT INTO `worldoverlay_overlay`
    (`overlay_key`, `map_id`, `enabled`, `base_spawn_policy`, `lifecycle_policy`, `comment`)
VALUES
    ('tele_city', 36, 1, 1, 0, 'Named overlay for Tele City on map 36')
ON DUPLICATE KEY UPDATE
    `map_id` = VALUES(`map_id`),
    `enabled` = VALUES(`enabled`),
    `base_spawn_policy` = VALUES(`base_spawn_policy`),
    `lifecycle_policy` = VALUES(`lifecycle_policy`),
    `comment` = VALUES(`comment`);

INSERT INTO `worldoverlay_destination`
    (`destination_key`, `map_id`, `position_x`, `position_y`, `position_z`, `orientation`, `instance_policy`, `overlay_key`, `enabled`, `comment`)
VALUES
    ('tele_city_moonwell', 36, -1544.160034, 730.984985, 8.554760, 5.803740, 2, 'tele_city', 1, 'Giant Moonwell landing point')
ON DUPLICATE KEY UPDATE
    `map_id` = VALUES(`map_id`),
    `position_x` = VALUES(`position_x`),
    `position_y` = VALUES(`position_y`),
    `position_z` = VALUES(`position_z`),
    `orientation` = VALUES(`orientation`),
    `instance_policy` = VALUES(`instance_policy`),
    `overlay_key` = VALUES(`overlay_key`),
    `enabled` = VALUES(`enabled`),
    `comment` = VALUES(`comment`);
