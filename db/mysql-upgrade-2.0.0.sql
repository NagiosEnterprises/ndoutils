-- BEGIN 2.0 MODS 

ALTER TABLE `nagios_notifications` ADD INDEX (`start_time`);
ALTER TABLE `nagios_contactnotifications` ADD INDEX (`start_time`);
ALTER TABLE `nagios_contactnotificationmethods` ADD INDEX (`start_time`);
ALTER TABLE `nagios_logentries` ADD INDEX (`logentry_time`); 
ALTER TABLE `nagios_acknowledgements` ADD INDEX (`entry_time`); 
ALTER TABLE `nagios_contacts` ADD `minimum_importance` int(11) NOT NULL default '0';
ALTER TABLE `nagios_hosts` ADD `importance` int(11) NOT NULL default '0';
ALTER TABLE `nagios_services` ADD `importance` int(11) NOT NULL default '0';

set @exist := (select count(*) from information_schema.statistics where table_name = 'nagios_logentries' and index_name = 'instance_id');
set @sqlstmt := if( @exist > 0, 'ALTER TABLE `nagios_logentries` DROP KEY `instance_id`', 'select ''INFO: Index does not exists.''');
PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;

ALTER TABLE `nagios_logentries` ADD UNIQUE KEY `instance_id` (`instance_id`,`logentry_time`,`entry_time`,`entry_time_usec`);

-- Table structure for table `nagios_service_parentservices`
--

CREATE TABLE IF NOT EXISTS `nagios_service_parentservices` (
  `service_parentservice_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `parent_service_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_parentservice_id`),
  UNIQUE KEY `instance_id` (`service_id`,`parent_service_object_id`)
) ENGINE=MyISAM  COMMENT='Parent services';

-- --------------------------------------------------------

--

-- END 2.0 MODS 

