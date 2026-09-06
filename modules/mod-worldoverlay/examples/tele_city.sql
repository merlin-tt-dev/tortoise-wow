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


-- Phase-0 isolation marker. This references an existing passive Turtle
-- gameobject_template (2011108, FancyDesk01) and is materialized only into the
-- tele_city WorldOverlay runtime. It never writes to the core gameobject table.
INSERT INTO `worldoverlay_gameobject`
    (`overlay_key`, `entry`, `position_x`, `position_y`, `position_z`, `orientation`,
     `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `enabled`, `comment`)
SELECT
    'tele_city', 2011108, -1540.800049, 731.000000, 8.554760, 5.803740,
    0.0, 0.0, 0.237433, -0.971404, 0, 100, 1, 1, 'Phase-0 WorldOverlay isolation marker'
WHERE NOT EXISTS (
    SELECT 1 FROM `worldoverlay_gameobject`
    WHERE `overlay_key` = 'tele_city' AND `comment` = 'Phase-0 WorldOverlay isolation marker'
);
