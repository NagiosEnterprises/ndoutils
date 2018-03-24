
CREATE TABLE IF NOT EXISTS `startup_hashes` (
  `startup_hash_id` int(11) NOT NULL,
  `hash` varchar(64) NOT NULL,
  `hash_timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, 
  PRIMARY KEY  (`startup_hash_id`)
) ENGINE=MyISAM  COMMENT='Startup hashes';