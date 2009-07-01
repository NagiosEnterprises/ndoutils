ALTER TABLE `nagios_hostchecks` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_hoststatus` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_servicechecks` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_servicestatus` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_statehistory` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;

ALTER TABLE `nagios_eventhandlers` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_systemcommands` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `nagios_notifications` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;

