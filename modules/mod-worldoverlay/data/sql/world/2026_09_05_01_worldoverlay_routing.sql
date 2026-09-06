-- mod-worldoverlay: future-ready WorldRouting destination resolution metadata.
--
-- Phase 0 implements resolver_type=0 (OWN) only. Values 1 and 2 are reserved so
-- later destinations can reuse existing Tortoise routing data without changing
-- the Destination/Binding/Executor architecture.

ALTER TABLE `worldoverlay_destination`
  ADD COLUMN `resolver_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=OWN, 1=SPELL_TARGET_POSITION, 2=AREATRIGGER_TELEPORT' AFTER `destination_key`,
  ADD COLUMN `resolver_ref` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Resolver-specific stable source id; unused for OWN' AFTER `resolver_type`;
