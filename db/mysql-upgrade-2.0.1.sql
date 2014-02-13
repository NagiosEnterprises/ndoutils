-- BEGIN 2.0.1 MODS 

set @exist := (select count(*) from information_schema.statistics where table_name = 'nagios_logentries' and index_name = 'instance_id');
set @sqlstmt := if( @exist > 0, 'ALTER TABLE `nagios_logentries` DROP KEY `instance_id`', 'select ''INFO: Index does not exists.''');
PREPARE stmt FROM @sqlstmt;
EXECUTE stmt;

ALTER TABLE `nagios_logentries` ADD UNIQUE KEY `instance_id` (`instance_id`,`logentry_time`,`entry_time`,`entry_time_usec`);

-- --------------------------------------------------------

--

-- END 2.0.1 MODS 

