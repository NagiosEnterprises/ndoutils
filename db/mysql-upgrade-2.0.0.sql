-- BEGIN 2.0 MODS 

ALTER TABLE `nagios_notifications` ADD INDEX (`start_time`);
ALTER TABLE `nagios_contactnotifications` ADD INDEX (`start_time`);
ALTER TABLE `nagios_contactnotificationmethods` ADD INDEX (`start_time`);
ALTER TABLE `nagios_logentries` ADD INDEX (`logentry_time`); 
ALTER TABLE `nagios_acknowledgements` ADD INDEX (`entry_time`); 

-- END 2.0 MODS 

