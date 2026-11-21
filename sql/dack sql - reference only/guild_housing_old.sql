-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server version:               8.4.9 - MySQL Community Server - GPL
-- Server OS:                    Win64
-- HeidiSQL Version:             12.21.0.7344
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

-- Dumping structure for table woi_characters.guild_house_masks
CREATE TABLE IF NOT EXISTS `guild_house_masks` (
  `guid` mediumint unsigned NOT NULL DEFAULT '0',
  `phasemask` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;

-- Dumping data for table woi_characters.guild_house_masks: ~40 rows (approximately)
DELETE FROM `guild_house_masks`;
INSERT INTO `guild_house_masks` (`guid`, `phasemask`) VALUES
	(3, 20),
	(7, 2),
	(8, 3),
	(25, 23),
	(34, 4),
	(65, 5),
	(97, 6),
	(98, 7),
	(107, 8),
	(117, 8),
	(129, 6),
	(135, 19),
	(141, 8),
	(156, 9),
	(160, 10),
	(175, 12),
	(192, 13),
	(201, 18),
	(204, 6),
	(208, 14),
	(225, 21),
	(231, 8),
	(245, 15),
	(255, 13),
	(272, 3),
	(273, 17),
	(299, 17),
	(304, 16),
	(307, 22),
	(309, 6),
	(310, 16),
	(340, 22),
	(347, 17),
	(354, 6),
	(355, 11),
	(455, 24),
	(470, 27),
	(477, 24),
	(480, 25),
	(534, 11);

-- Dumping structure for table woi_characters.guild_houses
CREATE TABLE IF NOT EXISTS `guild_houses` (
  `guildid` smallint unsigned NOT NULL DEFAULT '0',
  `mapid` smallint NOT NULL DEFAULT '0',
  `x` float(8,0) NOT NULL DEFAULT '0',
  `y` float(8,0) NOT NULL DEFAULT '0',
  `z` float(8,0) NOT NULL DEFAULT '0',
  `o` float(8,0) NOT NULL DEFAULT '0',
  `Comments` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Dumping data for table woi_characters.guild_houses: ~26 rows (approximately)
DELETE FROM `guild_houses`;
INSERT INTO `guild_houses` (`guildid`, `mapid`, `x`, `y`, `z`, `o`, `Comments`) VALUES
	(7, 1, -8218, -113, 9, 0, 'Dark Immortality & Immortality'),
	(34, 1, -6245, -923, -189, 0, 'Gaming Guru Gods'),
	(65, 1, -6240, -870, -178, 0, 'NewAge Alliance'),
	(97, 1, -7871, -100, 9, 0, 'legendary'),
	(98, 1, -6225, -917, -190, 0, 'Israel'),
	(107, 530, 8591, -8462, 1, 0, 'EnragedRabitz'),
	(117, 530, 8591, -8462, 1, 0, 'crtiz'),
	(129, 1, -7871, -100, 9, 0, 'Oblivion'),
	(141, 530, 8591, -8462, 1, 0, 'GameTime'),
	(156, 1, -7401, 9, 3, 0, 'NewAge Horde'),
	(160, 1, -8930, -377, 9, 0, 'The brotherhood'),
	(175, 1, -9094, -350, 9, 0, 'Indomitable'),
	(192, 1, -11426, -4702, 6, 0, 'Organized Chaos'),
	(204, 1, -7871, -100, 9, 0, 'Good Game Loosers'),
	(208, 0, -5580, 718, 486, 0, 'Bed Bath and Beyond'),
	(231, 530, 8587, -8468, 1, 0, 'We Downed Chuck Norris'),
	(245, 1, 1868, 1385, 142, 0, 'Fluffy Dragons'),
	(255, 1, -11426, -4702, 6, 0, 'Chaotic Order'),
	(272, 1, -8218, -113, 9, 0, 'Dark Immortality & Immortality'),
	(299, 1, -8429, 96, 9, 0, 'Will Work for Food'),
	(304, 1, -8197, -320, 9, 3, 'Asylum & Sanctuary'),
	(307, 0, -5877, 696, 486, 0, 'The Horde Gods'),
	(310, 1, -8197, -320, 9, 3, 'Asylum & Sanctuary'),
	(347, 0, -5877, 696, 486, 0, 'S.W.A.T\r\n'),
	(354, 1, -7871, -100, 9, 0, 'Supremacy'),
	(355, 1, -8930, -377, 9, 0, 'LionHeart');

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
