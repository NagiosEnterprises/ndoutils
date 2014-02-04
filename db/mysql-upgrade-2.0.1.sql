-- BEGIN 2.0.1 MODS 

ALTER TABLE `nagios_logentries` DROP KEY `instance_id`;
ALTER TABLE `nagios_logentries` ADD UNIQUE KEY `instance_id` (`instance_id`,`logentry_time`,`entry_time`,`entry_time_usec`);

-- --------------------------------------------------------

--

-- END 2.0.1 MODS 

