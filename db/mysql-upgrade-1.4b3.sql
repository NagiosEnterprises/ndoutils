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
