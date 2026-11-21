-- Guild house tables used by the vote teleporter (tele_npc_vote).
-- Structure only, and safe to run on a database that already has them (nothing is overwritten).
-- Import your existing guild house data separately.
--
-- guild_house_masks: guid = guild id, phasemask = the guild's house phase (> 1 = owns a house)
-- guild_houses:      guildid = guild id, mapid/x/y/z/o = where the house is

CREATE TABLE IF NOT EXISTS `guild_house_masks` (
  `guid` mediumint unsigned NOT NULL DEFAULT '0',
  `phasemask` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `guild_houses` (
  `guildid` smallint unsigned NOT NULL DEFAULT '0',
  `mapid` smallint NOT NULL DEFAULT '0',
  `x` float NOT NULL DEFAULT '0',
  `y` float NOT NULL DEFAULT '0',
  `z` float NOT NULL DEFAULT '0',
  `o` float NOT NULL DEFAULT '0',
  `Comments` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
