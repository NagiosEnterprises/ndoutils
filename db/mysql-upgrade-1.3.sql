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



ALTER TABLE `nagios_contactstatus` ENGINE=MyISAM COMMENT='Contact status'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Contact status'
ALTER TABLE `nagios_customvariables` DROP INDEX `object_id`; # was INDEX (`object_id`)
ALTER TABLE `nagios_customvariables` ENGINE=MyISAM COMMENT='Custom variables'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Custom variables'
ALTER TABLE `nagios_customvariablestatus` DROP INDEX `object_id_2`; # was INDEX (`object_id`)
ALTER TABLE `nagios_customvariablestatus` ADD INDEX `object_id_2` (`object_id`,`varname`);
ALTER TABLE `nagios_customvariablestatus` DROP INDEX `object_id`; # was INDEX (`object_id`,`varname`)
ALTER TABLE `nagios_customvariablestatus` ENGINE=MyISAM COMMENT='Custom variable status information'; # was ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Custom variable status information'
ALTER TABLE `nagios_host_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_hostescalation_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_service_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1
ALTER TABLE `nagios_serviceescalation_contacts` ENGINE=MyISAM DEFAULT CHARSET=ascii; # was ENGINE=MyISAM DEFAULT CHARSET=latin1

ALTER TABLE `nagios_customvariables` CHANGE `varname` `varname` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ,
CHANGE `varvalue` `varvalue` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL;

ALTER TABLE `nagios_customvariablestatus` CHANGE `varname` `varname` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ,
CHANGE `varvalue` `varvalue` VARCHAR( 255 ) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;


ALTER TABLE `nagios_services` ADD `host_object_id` INT( 11 ) NOT NULL DEFAULT '0' AFTER `config_type` ;


-- Start of mods from 1.4b3 --

ALTER TABLE `nagios_hosts` ADD `alias` VARCHAR( 64 ) NOT NULL AFTER `host_object_id` ;

-- End of mods from 1.4b3 --
-- BEGIN 1.4b5 MODS 

--                                                   
-- Table structure for table `nagios_host_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `nagios_host_contactgroups` (
  `host_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contactgroup_id`),
  UNIQUE KEY `instance_id` (`host_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Host contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `nagios_hostescalation_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `nagios_hostescalation_contactgroups` (
  `hostescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`hostescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Host escalation contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `nagios_service_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `nagios_service_contactgroups` (
  `service_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contactgroup_id`),
  UNIQUE KEY `instance_id` (`service_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Service contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `nagios_serviceescalation_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `nagios_serviceescalation_contactgroups` (
  `serviceescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`serviceescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Service escalation contact groups';

-- --------------------------------------------------------

ALTER TABLE `nagios_acknowledgements` TYPE = innodb;
ALTER TABLE `nagios_commands` TYPE = innodb;
ALTER TABLE `nagios_commenthistory` TYPE = innodb;
ALTER TABLE `nagios_comments` TYPE = innodb;
ALTER TABLE `nagios_configfiles` TYPE = innodb;
ALTER TABLE `nagios_configfilevariables` TYPE = innodb;
ALTER TABLE `nagios_conninfo` TYPE = innodb;
ALTER TABLE `nagios_contact_addresses` TYPE = innodb;
ALTER TABLE `nagios_contact_notificationcommands` TYPE = innodb;
ALTER TABLE `nagios_contactgroup_members` TYPE = innodb;
ALTER TABLE `nagios_contactgroups` TYPE = innodb;
ALTER TABLE `nagios_contactnotificationmethods` TYPE = innodb;
ALTER TABLE `nagios_contactnotifications` TYPE = innodb;
ALTER TABLE `nagios_contacts` TYPE = innodb;
ALTER TABLE `nagios_contactstatus` TYPE = innodb;
ALTER TABLE `nagios_customvariables` TYPE = innodb;
ALTER TABLE `nagios_customvariablestatus` TYPE = innodb;
ALTER TABLE `nagios_dbversion` TYPE = innodb;
ALTER TABLE `nagios_downtimehistory` TYPE = innodb;
ALTER TABLE `nagios_eventhandlers` TYPE = innodb;
ALTER TABLE `nagios_externalcommands` TYPE = innodb;
ALTER TABLE `nagios_flappinghistory` TYPE = innodb;
ALTER TABLE `nagios_host_contactgroups` TYPE = innodb;
ALTER TABLE `nagios_host_contacts` TYPE = innodb;
ALTER TABLE `nagios_host_parenthosts` TYPE = innodb;
ALTER TABLE `nagios_hostchecks` TYPE = innodb;
ALTER TABLE `nagios_hostdependencies` TYPE = innodb;
ALTER TABLE `nagios_hostescalation_contactgroups` TYPE = innodb;
ALTER TABLE `nagios_hostescalation_contacts` TYPE = innodb;
ALTER TABLE `nagios_hostescalations` TYPE = innodb;
ALTER TABLE `nagios_hostgroup_members` TYPE = innodb;
ALTER TABLE `nagios_hostgroups` TYPE = innodb;
ALTER TABLE `nagios_hosts` TYPE = innodb;
ALTER TABLE `nagios_hoststatus` TYPE = innodb;
ALTER TABLE `nagios_instances` TYPE = innodb;
ALTER TABLE `nagios_logentries` TYPE = innodb;
ALTER TABLE `nagios_notifications` TYPE = innodb;
ALTER TABLE `nagios_objects` TYPE = innodb;
ALTER TABLE `nagios_processevents` TYPE = innodb;
ALTER TABLE `nagios_programstatus` TYPE = innodb;
ALTER TABLE `nagios_runtimevariables` TYPE = innodb;
ALTER TABLE `nagios_scheduleddowntime` TYPE = innodb;
ALTER TABLE `nagios_service_contactgroups` TYPE = innodb;
ALTER TABLE `nagios_service_contacts` TYPE = innodb;
ALTER TABLE `nagios_servicechecks` TYPE = innodb;
ALTER TABLE `nagios_servicedependencies` TYPE = innodb;
ALTER TABLE `nagios_serviceescalation_contactgroups` TYPE = innodb;
ALTER TABLE `nagios_serviceescalation_contacts` TYPE = innodb;
ALTER TABLE `nagios_serviceescalations` TYPE = innodb;
ALTER TABLE `nagios_servicegroup_members` TYPE = innodb;
ALTER TABLE `nagios_servicegroups` TYPE = innodb;
ALTER TABLE `nagios_services` TYPE = innodb;
ALTER TABLE `nagios_servicestatus` TYPE = innodb;
ALTER TABLE `nagios_statehistory` TYPE = innodb;
ALTER TABLE `nagios_systemcommands` TYPE = innodb;
ALTER TABLE `nagios_timedeventqueue` TYPE = innodb;
ALTER TABLE `nagios_timedevents` TYPE = innodb;
ALTER TABLE `nagios_timeperiod_timeranges` TYPE = innodb;
ALTER TABLE `nagios_timeperiods` TYPE = innodb;

ALTER TABLE `nagios_statehistory` ADD `last_state` SMALLINT DEFAULT '-1' NOT NULL AFTER `max_check_attempts` ,
ADD `last_hard_state` SMALLINT DEFAULT '-1' NOT NULL AFTER `last_state` ;

-- END 1.4b5 MODS 
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
