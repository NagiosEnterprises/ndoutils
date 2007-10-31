-- BEGIN 1.4b7 MODS 

ALTER TABLE `nagios_configfilevariables` DROP INDEX `instance_id`; 
ALTER TABLE `nagios_configfilevariables` ADD INDEX ( `instance_id` , `configfile_id` ) ;

ALTER TABLE `nagios_timedeventqueue` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_statehistory` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `nagios_servicestatus` ADD INDEX ( `instance_id` , `service_object_id` ) ;

ALTER TABLE `nagios_processevents` ADD INDEX ( `instance_id` , `event_type` ) ;

ALTER TABLE `nagios_logentries` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_hoststatus` ADD INDEX ( `instance_id` , `host_object_id` ) ;

ALTER TABLE `nagios_flappinghistory` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `nagios_externalcommands` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_customvariablestatus` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_contactstatus` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_conninfo` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_acknowledgements` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `nagios_objects` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `nagios_logentries` ADD INDEX ( `logentry_time` ) ;

ALTER TABLE `nagios_commenthistory` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_comments` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_downtimehistory` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_externalcommands` CHANGE `command_args` `command_args` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_hostchecks` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `nagios_hostchecks` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_hoststatus` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `nagios_hoststatus` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_logentries` CHANGE `logentry_data` `logentry_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_scheduleddowntime` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_servicechecks` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `nagios_servicechecks` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_servicestatus` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `nagios_servicestatus` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_statehistory` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `nagios_processevents` ADD INDEX ( `event_time` , `event_time_usec` ) ;

ALTER TABLE `nagios_hoststatus` ADD INDEX ( `current_state` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `state_type` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `last_check` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `notifications_enabled` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `problem_has_been_acknowledged` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `passive_checks_enabled` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `active_checks_enabled` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `event_handler_enabled` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `flap_detection_enabled` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `is_flapping` ) ;
ALTER TABLE `nagios_hoststatus` ADD INDEX ( `scheduled_downtime_depth` ) ;

ALTER TABLE `nagios_servicestatus` ADD INDEX ( `current_state` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `last_check` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `notifications_enabled` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `problem_has_been_acknowledged` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `passive_checks_enabled` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `active_checks_enabled` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `event_handler_enabled` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `flap_detection_enabled` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `is_flapping` ) ;
ALTER TABLE `nagios_servicestatus` ADD INDEX ( `scheduled_downtime_depth` ) ;

ALTER TABLE `nagios_statehistory` ADD INDEX ( `state_time` , `state_time_usec` ) ;

ALTER TABLE `nagios_timedeventqueue` ADD INDEX ( `event_type` ) ;
ALTER TABLE `nagios_timedeventqueue` ADD INDEX ( `scheduled_time` ) ;

ALTER TABLE `nagios_logentries` ADD INDEX ( `entry_time` ) ;
ALTER TABLE `nagios_logentries` ADD INDEX ( `entry_time_usec` ) ;

ALTER TABLE `nagios_externalcommands` ADD INDEX ( `entry_time` ) ;

-- END 1.4b7 MODS 
