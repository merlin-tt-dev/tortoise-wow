-- mod-worldoverlay initial world schema
--
-- Design goals:
--   * persistent logical overlay keys
--   * no persistent dependency on runtime instance ids
--   * module-owned overlay spawns separate from base creature/gameobject tables
--   * generic teleport destinations and simple source bindings
--
-- Runtime code is not implemented yet. This migration only establishes the
-- module-owned persistence model.

CREATE TABLE IF NOT EXISTS `worldoverlay_overlay` (
  `overlay_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `overlay_key` VARCHAR(64) NOT NULL,
  `map_id` INT UNSIGNED NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `base_spawn_policy` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0=NONE, 1=INHERIT',
  `lifecycle_policy` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=PERSISTENT, 1=WHEN_EMPTY, 2=MANUAL',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`overlay_id`),
  UNIQUE KEY `uk_worldoverlay_overlay_key` (`overlay_key`),
  KEY `idx_worldoverlay_overlay_map` (`map_id`, `enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `worldoverlay_destination` (
  `destination_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `destination_key` VARCHAR(64) NOT NULL,
  `map_id` INT UNSIGNED NOT NULL,
  `position_x` FLOAT NOT NULL,
  `position_y` FLOAT NOT NULL,
  `position_z` FLOAT NOT NULL,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  `instance_policy` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=AUTO, 1=CURRENT, 2=OVERLAY',
  `overlay_key` VARCHAR(64) NOT NULL DEFAULT '',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`destination_id`),
  UNIQUE KEY `uk_worldoverlay_destination_key` (`destination_key`),
  KEY `idx_worldoverlay_destination_overlay` (`overlay_key`),
  KEY `idx_worldoverlay_destination_map` (`map_id`, `enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `worldoverlay_gameobject` (
  `spawn_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `overlay_key` VARCHAR(64) NOT NULL,
  `entry` INT UNSIGNED NOT NULL,
  `position_x` FLOAT NOT NULL,
  `position_y` FLOAT NOT NULL,
  `position_z` FLOAT NOT NULL,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  `rotation0` FLOAT NOT NULL DEFAULT 0,
  `rotation1` FLOAT NOT NULL DEFAULT 0,
  `rotation2` FLOAT NOT NULL DEFAULT 0,
  `rotation3` FLOAT NOT NULL DEFAULT 1,
  `spawntimesecs` INT NOT NULL DEFAULT 300,
  `animprogress` TINYINT UNSIGNED NOT NULL DEFAULT 100,
  `state` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`spawn_id`),
  KEY `idx_worldoverlay_gameobject_overlay` (`overlay_key`, `enabled`),
  KEY `idx_worldoverlay_gameobject_entry` (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `worldoverlay_creature` (
  `spawn_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `overlay_key` VARCHAR(64) NOT NULL,
  `entry` INT UNSIGNED NOT NULL,
  `position_x` FLOAT NOT NULL,
  `position_y` FLOAT NOT NULL,
  `position_z` FLOAT NOT NULL,
  `orientation` FLOAT NOT NULL DEFAULT 0,
  `spawntimesecs` INT NOT NULL DEFAULT 300,
  `spawndist` FLOAT NOT NULL DEFAULT 0,
  `movement_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`spawn_id`),
  KEY `idx_worldoverlay_creature_overlay` (`overlay_key`, `enabled`),
  KEY `idx_worldoverlay_creature_entry` (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `worldoverlay_teleport_binding` (
  `source_type` TINYINT UNSIGNED NOT NULL COMMENT '1=ITEM, 2=GAMEOBJECT, 3=CREATURE',
  `source_entry` INT UNSIGNED NOT NULL,
  `destination_key` VARCHAR(64) NOT NULL,
  `cooldown_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `flags` INT UNSIGNED NOT NULL DEFAULT 0,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`source_type`, `source_entry`),
  KEY `idx_worldoverlay_binding_destination` (`destination_key`),
  KEY `idx_worldoverlay_binding_enabled` (`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
