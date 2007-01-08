ALTER TABLE `ndo_conninfo` CHANGE `instance_id` `instance_id` SMALLINT NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_services` ADD `notes` VARCHAR( 255 ) NOT NULL ,
ADD `notes_url` VARCHAR( 255 ) NOT NULL ,
ADD `action_url` VARCHAR( 255 ) NOT NULL ,
ADD `icon_image` VARCHAR( 255 ) NOT NULL ,
ADD `icon_image_alt` VARCHAR( 255 ) NOT NULL ; 

DROP TABLE `ndo_serviceextinfo` ;

ALTER TABLE `ndo_hosts` ADD `notes` VARCHAR( 255 ) NOT NULL ,
ADD `notes_url` VARCHAR( 255 ) NOT NULL ,
ADD `action_url` VARCHAR( 255 ) NOT NULL ,
ADD `icon_image` VARCHAR( 255 ) NOT NULL ,
ADD `icon_image_alt` VARCHAR( 255 ) NOT NULL ,
ADD `vrml_image` VARCHAR( 255 ) NOT NULL ,
ADD `statusmap_image` VARCHAR( 255 ) NOT NULL ,
ADD `have_2d_coords` SMALLINT NOT NULL ,
ADD `x_2d` SMALLINT NOT NULL ,
ADD `y_2d` SMALLINT NOT NULL ,
ADD `have_3d_coords` SMALLINT NOT NULL ,
ADD `x_3d` DOUBLE NOT NULL ,
ADD `y_3d` DOUBLE NOT NULL ,
ADD `z_3d` DOUBLE NOT NULL ;

DROP TABLE `ndo_hostextinfo`;

ALTER TABLE `ndo_hostescalations` CHANGE `notification_interval` `notification_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_serviceescalations` CHANGE `notification_interval` `notification_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_services` DROP `parallelize_check` ;

ALTER TABLE `ndo_services` CHANGE `check_interval` `check_interval` DOUBLE NOT NULL DEFAULT '0',
CHANGE `retry_interval` `retry_interval` DOUBLE NOT NULL DEFAULT '0',
CHANGE `notification_interval` `notification_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_hosts` CHANGE `check_interval` `check_interval` DOUBLE NOT NULL DEFAULT '0',
CHANGE `notification_interval` `notification_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_hoststatus` CHANGE `normal_check_interval` `normal_check_interval` DOUBLE NOT NULL DEFAULT '0',
CHANGE `retry_check_interval` `retry_check_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_servicestatus` CHANGE `normal_check_interval` `normal_check_interval` DOUBLE NOT NULL DEFAULT '0',
CHANGE `retry_check_interval` `retry_check_interval` DOUBLE NOT NULL DEFAULT '0' ;

ALTER TABLE `ndo_services` ADD `first_notification_delay` DOUBLE NOT NULL AFTER `max_check_attempts` ;

ALTER TABLE `ndo_hosts` ADD `first_notification_delay` DOUBLE NOT NULL AFTER `max_check_attempts` ;

ALTER TABLE `ndo_hosts` ADD `notify_on_downtime` SMALLINT NOT NULL AFTER `notify_on_flapping` ;

ALTER TABLE `ndo_services` ADD `notify_on_downtime` SMALLINT NOT NULL AFTER `notify_on_flapping` ;

ALTER TABLE `ndo_contacts` ADD `host_notifications_enabled` SMALLINT NOT NULL AFTER `service_timeperiod_object_id` ,
ADD `service_notifications_enabled` SMALLINT NOT NULL AFTER `host_notifications_enabled` ,
ADD `can_submit_commands` SMALLINT NOT NULL AFTER `service_notifications_enabled` ;

ALTER TABLE `ndo_contacts` ADD `notify_host_downtime` SMALLINT NOT NULL AFTER `notify_host_flapping` ;

ALTER TABLE `ndo_contacts` ADD `notify_service_downtime` SMALLINT NOT NULL AFTER `notify_service_flapping` ;

ALTER TABLE `ndo_hosts` ADD `retry_interval` DOUBLE NOT NULL AFTER `check_interval` ;

ALTER TABLE `ndo_hosts` ADD `flap_detection_on_up` SMALLINT NOT NULL AFTER `flap_detection_enabled` ,
ADD `flap_detection_on_down` SMALLINT NOT NULL AFTER `flap_detection_on_up` ,
ADD `flap_detection_on_unreachable` SMALLINT NOT NULL AFTER `flap_detection_on_down` ;

ALTER TABLE `ndo_services` ADD `flap_detection_on_ok` SMALLINT NOT NULL AFTER `flap_detection_enabled` ,
ADD `flap_detection_on_warning` SMALLINT NOT NULL AFTER `flap_detection_on_ok` ,
ADD `flap_detection_on_unknown` SMALLINT NOT NULL AFTER `flap_detection_on_warning` ,
ADD `flap_detection_on_critical` SMALLINT NOT NULL AFTER `flap_detection_on_unknown` ;

ALTER TABLE `ndo_services` ADD `display_name` VARCHAR( 64 ) NOT NULL AFTER `service_object_id` ;

ALTER TABLE `ndo_hosts` ADD `display_name` VARCHAR( 64 ) NOT NULL AFTER `host_object_id` ;

ALTER TABLE `ndo_servicestatus` ADD `check_timeperiod_object_id` INT NOT NULL ;

ALTER TABLE `ndo_hoststatus` ADD `check_timeperiod_object_id` INT NOT NULL ;

ALTER TABLE `ndo_servicedependencies` ADD `timeperiod_object_id` INT NOT NULL AFTER `inherits_parent` ;

ALTER TABLE `ndo_hostdependencies` ADD `timeperiod_object_id` INT NOT NULL AFTER `inherits_parent` ;

CREATE TABLE `ndo_contactstatus` (
`contactstatus_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`contact_object_id` INT NOT NULL ,
`status_update_time` DATETIME NOT NULL ,
`host_notifications_enabled` SMALLINT NOT NULL ,
`service_notifications_enabled` SMALLINT NOT NULL ,
`last_host_notification` DATETIME NOT NULL ,
`last_service_notification` DATETIME NOT NULL ,
`modified_attributes` INT NOT NULL ,
`modified_host_attributes` INT NOT NULL ,
`modified_service_attributes` INT NOT NULL ,
PRIMARY KEY ( `contactstatus_id` ) ,
UNIQUE (
`contact_object_id`
)
) TYPE = MYISAM COMMENT = 'Contact status';

CREATE TABLE `ndo_customvariables` (
`customvariable_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`object_id` INT NOT NULL ,
`config_type` SMALLINT NOT NULL ,
`has_been_modified` SMALLINT NOT NULL ,
`varname` VARCHAR( 255 ) NOT NULL ,
`varvalue` VARCHAR( 255 ) NOT NULL ,
PRIMARY KEY ( `customvariable_id` )
) TYPE = MYISAM COMMENT = 'Custom variables';

DROP TABLE `ndo_customobjectvariables` ;

CREATE TABLE `ndo_customvariablestatus` (
`customvariablestatus_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`object_id` INT NOT NULL ,
`status_update_time` DATETIME NOT NULL ,
`has_been_modified` SMALLINT NOT NULL ,
`varname` VARCHAR( 255 ) NOT NULL ,
`varvalue` VARCHAR( 255 ) NOT NULL ,
PRIMARY KEY ( `customvariablestatus_id` )
) TYPE = MYISAM COMMENT = 'Custom variable status information';

ALTER TABLE `ndo_customvariablestatus` ADD UNIQUE (
`object_id` ,
`varname`
);

ALTER TABLE `ndo_customvariablestatus` ADD INDEX ( `object_id` ) ;

ALTER TABLE `ndo_customvariablestatus` ADD INDEX ( `varname` ) ;

ALTER TABLE `ndo_customvariables` ADD INDEX ( `object_id` ) ;

ALTER TABLE `ndo_customvariables` ADD INDEX ( `varname` ) ;

ALTER TABLE `ndo_customvariables` ADD UNIQUE (
`object_id` ,
`config_type` ,
`varname`
);

DROP TABLE `ndo_serviceescalation_contactgroups`;

CREATE TABLE `nagios_serviceescalation_contacts` (
`serviceescalation_contact_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`serviceescalation_id` INT NOT NULL ,
`contact_object_id` INT NOT NULL ,
PRIMARY KEY ( `serviceescalation_contact_id` ) ,
UNIQUE (
`instance_id`
)
) TYPE = MYISAM ;

DROP TABLE `ndo_hostescalation_contactgroups`;

CREATE TABLE `nagios_hostescalation_contacts` (
`hostescalation_contact_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`hostescalation_id` INT NOT NULL ,
`contact_object_id` INT NOT NULL ,
PRIMARY KEY ( `hostescalation_contact_id` ) ,
UNIQUE (
`instance_id`
)
) TYPE = MYISAM ;

DROP TABLE `ndo_host_contactgroups`;

CREATE TABLE `nagios_host_contacts` (
`host_contact_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`host_id` INT NOT NULL ,
`contact_object_id` INT NOT NULL ,
PRIMARY KEY ( `host_contact_id` ) ,
UNIQUE (
`instance_id`
)
) TYPE = MYISAM ;

DROP TABLE `ndo_service_contactgroups`;

CREATE TABLE `nagios_service_contacts` (
`service_contact_id` INT NOT NULL AUTO_INCREMENT ,
`instance_id` SMALLINT NOT NULL ,
`service_id` INT NOT NULL ,
`contact_object_id` INT NOT NULL ,
PRIMARY KEY ( `service_contact_id` ) ,
UNIQUE (
`instance_id`
)
) TYPE = MYISAM ;



ALTER TABLE `ndo_acknowledgements` RENAME `nagios_acknowledgements` ;
ALTER TABLE `ndo_commands` RENAME `nagios_commands` ;
ALTER TABLE `ndo_commenthistory` RENAME `nagios_commenthistory` ;
ALTER TABLE `ndo_comments` RENAME `nagios_comments` ;
ALTER TABLE `ndo_configfiles` RENAME `nagios_configfiles` ;
ALTER TABLE `ndo_configfilevariables` RENAME `nagios_configfilevariables` ;
ALTER TABLE `ndo_conninfo` RENAME `nagios_conninfo` ;
ALTER TABLE `ndo_contact_addresses` RENAME `nagios_contact_addresses` ;
ALTER TABLE `ndo_contact_notificationcommands` RENAME `nagios_contact_notificationcommands` ;
ALTER TABLE `ndo_contactgroup_members` RENAME `nagios_contactgroup_members` ;
ALTER TABLE `ndo_contactgroups` RENAME `nagios_contactgroups` ;
ALTER TABLE `ndo_contactnotificationmethods` RENAME `nagios_contactnotificationmethods` ;
ALTER TABLE `ndo_contactnotifications` RENAME `nagios_contactnotifications` ;
ALTER TABLE `ndo_contacts` RENAME `nagios_contacts` ;
ALTER TABLE `ndo_contactstatus` RENAME `nagios_contactstatus` ;
ALTER TABLE `ndo_customvariables` RENAME `nagios_customvariables` ;
ALTER TABLE `ndo_customvariablestatus` RENAME `nagios_customvariablestatus` ;
ALTER TABLE `ndo_downtimehistory` RENAME `nagios_downtimehistory` ;
ALTER TABLE `ndo_eventhandlers` RENAME `nagios_eventhandlers` ;
ALTER TABLE `ndo_externalcommands` RENAME `nagios_externalcommands` ;
ALTER TABLE `ndo_flappinghistory` RENAME `nagios_flappinghistory` ;
ALTER TABLE `ndo_host_parenthosts` RENAME `nagios_host_parenthosts` ;
ALTER TABLE `ndo_hostchecks` RENAME `nagios_hostchecks` ;
ALTER TABLE `ndo_hostdependencies` RENAME `nagios_hostdependencies` ;
ALTER TABLE `ndo_hostescalations` RENAME `nagios_hostescalations` ;
ALTER TABLE `ndo_hostgroup_members` RENAME `nagios_hostgroup_members` ;
ALTER TABLE `ndo_hostgroups` RENAME `nagios_hostgroups` ;
ALTER TABLE `ndo_hosts` RENAME `nagios_hosts` ;
ALTER TABLE `ndo_hoststatus` RENAME `nagios_hoststatus` ;
ALTER TABLE `ndo_instances` RENAME `nagios_instances` ;
ALTER TABLE `ndo_logentries` RENAME `nagios_logentries` ;
ALTER TABLE `ndo_notifications` RENAME `nagios_notifications` ;
ALTER TABLE `ndo_objects` RENAME `nagios_objects` ;
ALTER TABLE `ndo_processevents` RENAME `nagios_processevents` ;
ALTER TABLE `ndo_programstatus` RENAME `nagios_programstatus` ;
ALTER TABLE `ndo_runtimevariables` RENAME `nagios_runtimevariables` ;
ALTER TABLE `ndo_scheduleddowntime` RENAME `nagios_scheduleddowntime` ;
ALTER TABLE `ndo_servicechecks` RENAME `nagios_servicechecks` ;
ALTER TABLE `ndo_servicedependencies` RENAME `nagios_servicedependencies` ;
ALTER TABLE `ndo_serviceescalations` RENAME `nagios_serviceescalations` ;
ALTER TABLE `ndo_servicegroup_members` RENAME `nagios_servicegroup_members` ;
ALTER TABLE `ndo_servicegroups` RENAME `nagios_servicegroups` ;
ALTER TABLE `ndo_services` RENAME `nagios_services` ;
ALTER TABLE `ndo_servicestatus` RENAME `nagios_servicestatus` ;
ALTER TABLE `ndo_statehistory` RENAME `nagios_statehistory` ;
ALTER TABLE `ndo_systemcommands` RENAME `nagios_systemcommands` ;
ALTER TABLE `ndo_timedeventqueue` RENAME `nagios_timedeventqueue` ;
ALTER TABLE `ndo_timedevents` RENAME `nagios_timedevents` ;
ALTER TABLE `ndo_timeperiod_timeranges` RENAME `nagios_timeperiod_timeranges` ;
ALTER TABLE `ndo_timeperiods` RENAME `nagios_timeperiods` ;



ALTER TABLE `nagios_processevents` CHANGE `program_version` `program_version` VARCHAR( 20 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL;

ALTER TABLE `nagios_statehistory` CHANGE `max_attempts` `max_check_attempts` SMALLINT( 6 ) NOT NULL DEFAULT '0';

ALTER TABLE `nagios_statehistory` CHANGE `current_attempt` `current_check_attempt` SMALLINT( 6 ) NOT NULL DEFAULT '0';



ALTER TABLE `nagios_contactstatus` ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Contact status'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Contact status'
ALTER TABLE `nagios_customvariables` DROP INDEX `object_id`; # was INDEX (`object_id`)
ALTER TABLE `nagios_customvariables` ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Custom variables'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Custom variables'
ALTER TABLE `nagios_customvariablestatus` DROP INDEX `object_id_2`; # was INDEX (`object_id`)
ALTER TABLE `nagios_customvariablestatus` ADD INDEX `object_id_2` (`object_id`,`varname`);
ALTER TABLE `nagios_customvariablestatus` DROP INDEX `object_id`; # was INDEX (`object_id`,`varname`)
ALTER TABLE `nagios_customvariablestatus` ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Custom variable status information'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Custom variable status information'
ALTER TABLE `nagios_host_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_hostescalation_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_service_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_serviceescalation_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1

ALTER TABLE `nagios_customvariables` CHANGE `varname` `varname` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ,
CHANGE `varvalue` `varvalue` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL;

ALTER TABLE `nagios_customvariablestatus` CHANGE `varname` `varname` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ,
CHANGE `varvalue` `varvalue` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;


