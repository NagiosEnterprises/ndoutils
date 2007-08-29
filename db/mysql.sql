-- phpMyAdmin SQL Dump
-- version 2.6.4-pl4
-- http://www.phpmyadmin.net
-- 
-- Host: localhost
-- Generation Time: Aug 29, 2007 at 09:08 AM
-- Server version: 4.1.20
-- PHP Version: 5.0.4
-- 
-- Database: `nagios`
-- 

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_acknowledgements`
-- 

CREATE TABLE `nagios_acknowledgements` (
  `acknowledgement_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `entry_time_usec` int(11) NOT NULL default '0',
  `acknowledgement_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_sticky` smallint(6) NOT NULL default '0',
  `persistent_comment` smallint(6) NOT NULL default '0',
  `notify_contacts` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`acknowledgement_id`)
) ENGINE=InnoDB COMMENT='Current and historical host and service acknowledgements';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_commands`
-- 

CREATE TABLE `nagios_commands` (
  `command_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `command_line` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`command_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`config_type`)
) ENGINE=InnoDB COMMENT='Command definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_commenthistory`
-- 

CREATE TABLE `nagios_commenthistory` (
  `commenthistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `entry_time_usec` int(11) NOT NULL default '0',
  `comment_type` smallint(6) NOT NULL default '0',
  `entry_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `comment_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `internal_comment_id` int(11) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_persistent` smallint(6) NOT NULL default '0',
  `comment_source` smallint(6) NOT NULL default '0',
  `expires` smallint(6) NOT NULL default '0',
  `expiration_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `deletion_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `deletion_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`commenthistory_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`comment_time`,`internal_comment_id`)
) ENGINE=InnoDB COMMENT='Historical host and service comments';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_comments`
-- 

CREATE TABLE `nagios_comments` (
  `comment_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `entry_time_usec` int(11) NOT NULL default '0',
  `comment_type` smallint(6) NOT NULL default '0',
  `entry_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `comment_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `internal_comment_id` int(11) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_persistent` smallint(6) NOT NULL default '0',
  `comment_source` smallint(6) NOT NULL default '0',
  `expires` smallint(6) NOT NULL default '0',
  `expiration_time` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`comment_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`comment_time`,`internal_comment_id`)
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_configfiles`
-- 

CREATE TABLE `nagios_configfiles` (
  `configfile_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `configfile_type` smallint(6) NOT NULL default '0',
  `configfile_path` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`configfile_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`configfile_type`,`configfile_path`)
) ENGINE=InnoDB COMMENT='Configuration files';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_configfilevariables`
-- 

CREATE TABLE `nagios_configfilevariables` (
  `configfilevariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `configfile_id` int(11) NOT NULL default '0',
  `varname` varchar(64) NOT NULL default '',
  `varvalue` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`configfilevariable_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`configfile_id`,`varname`)
) ENGINE=InnoDB COMMENT='Configuration file variables';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_conninfo`
-- 

CREATE TABLE `nagios_conninfo` (
  `conninfo_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `agent_name` varchar(32) NOT NULL default '',
  `agent_version` varchar(8) NOT NULL default '',
  `disposition` varchar(16) NOT NULL default '',
  `connect_source` varchar(16) NOT NULL default '',
  `connect_type` varchar(16) NOT NULL default '',
  `connect_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `disconnect_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_checkin_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `data_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `data_end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `bytes_processed` int(11) NOT NULL default '0',
  `lines_processed` int(11) NOT NULL default '0',
  `entries_processed` int(11) NOT NULL default '0',
  PRIMARY KEY  (`conninfo_id`)
) ENGINE=InnoDB COMMENT='NDO2DB daemon connection information';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contact_addresses`
-- 

CREATE TABLE `nagios_contact_addresses` (
  `contact_address_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_id` int(11) NOT NULL default '0',
  `address_number` smallint(6) NOT NULL default '0',
  `address` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contact_address_id`),
  UNIQUE KEY `contact_id` (`contact_id`,`address_number`)
) ENGINE=InnoDB COMMENT='Contact addresses';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contact_notificationcommands`
-- 

CREATE TABLE `nagios_contact_notificationcommands` (
  `contact_notificationcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_id` int(11) NOT NULL default '0',
  `notification_type` smallint(6) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contact_notificationcommand_id`),
  UNIQUE KEY `contact_id` (`contact_id`,`notification_type`,`command_object_id`,`command_args`)
) ENGINE=InnoDB COMMENT='Contact host and service notification commands';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contactgroup_members`
-- 

CREATE TABLE `nagios_contactgroup_members` (
  `contactgroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contactgroup_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactgroup_member_id`),
  UNIQUE KEY `instance_id` (`contactgroup_id`,`contact_object_id`)
) ENGINE=InnoDB COMMENT='Contactgroup members';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contactgroups`
-- 

CREATE TABLE `nagios_contactgroups` (
  `contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contactgroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`contactgroup_object_id`)
) ENGINE=InnoDB COMMENT='Contactgroup definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contactnotificationmethods`
-- 

CREATE TABLE `nagios_contactnotificationmethods` (
  `contactnotificationmethod_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contactnotification_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contactnotificationmethod_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`contactnotification_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical record of contact notification methods';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contactnotifications`
-- 

CREATE TABLE `nagios_contactnotifications` (
  `contactnotification_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `notification_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactnotification_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`contact_object_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical record of contact notifications';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contacts`
-- 

CREATE TABLE `nagios_contacts` (
  `contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `alias` varchar(64) NOT NULL default '',
  `email_address` varchar(255) NOT NULL default '',
  `pager_address` varchar(64) NOT NULL default '',
  `host_timeperiod_object_id` int(11) NOT NULL default '0',
  `service_timeperiod_object_id` int(11) NOT NULL default '0',
  `host_notifications_enabled` smallint(6) NOT NULL default '0',
  `service_notifications_enabled` smallint(6) NOT NULL default '0',
  `can_submit_commands` smallint(6) NOT NULL default '0',
  `notify_service_recovery` smallint(6) NOT NULL default '0',
  `notify_service_warning` smallint(6) NOT NULL default '0',
  `notify_service_unknown` smallint(6) NOT NULL default '0',
  `notify_service_critical` smallint(6) NOT NULL default '0',
  `notify_service_flapping` smallint(6) NOT NULL default '0',
  `notify_service_downtime` smallint(6) NOT NULL default '0',
  `notify_host_recovery` smallint(6) NOT NULL default '0',
  `notify_host_down` smallint(6) NOT NULL default '0',
  `notify_host_unreachable` smallint(6) NOT NULL default '0',
  `notify_host_flapping` smallint(6) NOT NULL default '0',
  `notify_host_downtime` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`contact_object_id`)
) ENGINE=InnoDB COMMENT='Contact definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_contactstatus`
-- 

CREATE TABLE `nagios_contactstatus` (
  `contactstatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `host_notifications_enabled` smallint(6) NOT NULL default '0',
  `service_notifications_enabled` smallint(6) NOT NULL default '0',
  `last_host_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_service_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `modified_attributes` int(11) NOT NULL default '0',
  `modified_host_attributes` int(11) NOT NULL default '0',
  `modified_service_attributes` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactstatus_id`),
  UNIQUE KEY `contact_object_id` (`contact_object_id`)
) ENGINE=InnoDB COMMENT='Contact status';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_customvariables`
-- 

CREATE TABLE `nagios_customvariables` (
  `customvariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `has_been_modified` smallint(6) NOT NULL default '0',
  `varname` varchar(255) NOT NULL default '',
  `varvalue` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`customvariable_id`),
  UNIQUE KEY `object_id_2` (`object_id`,`config_type`,`varname`),
  KEY `varname` (`varname`)
) ENGINE=InnoDB COMMENT='Custom variables';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_customvariablestatus`
-- 

CREATE TABLE `nagios_customvariablestatus` (
  `customvariablestatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `has_been_modified` smallint(6) NOT NULL default '0',
  `varname` varchar(255) NOT NULL default '',
  `varvalue` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`customvariablestatus_id`),
  UNIQUE KEY `object_id_2` (`object_id`,`varname`),
  KEY `varname` (`varname`)
) ENGINE=InnoDB COMMENT='Custom variable status information';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_dbversion`
-- 

CREATE TABLE `nagios_dbversion` (
  `name` varchar(10) NOT NULL default '',
  `version` varchar(10) NOT NULL default ''
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_downtimehistory`
-- 

CREATE TABLE `nagios_downtimehistory` (
  `downtimehistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `downtime_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `internal_downtime_id` int(11) NOT NULL default '0',
  `triggered_by_id` int(11) NOT NULL default '0',
  `is_fixed` smallint(6) NOT NULL default '0',
  `duration` smallint(6) NOT NULL default '0',
  `scheduled_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `scheduled_end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `was_started` smallint(6) NOT NULL default '0',
  `actual_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `actual_start_time_usec` int(11) NOT NULL default '0',
  `actual_end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `actual_end_time_usec` int(11) NOT NULL default '0',
  `was_cancelled` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`downtimehistory_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`entry_time`,`internal_downtime_id`)
) ENGINE=InnoDB COMMENT='Historical scheduled host and service downtime';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_eventhandlers`
-- 

CREATE TABLE `nagios_eventhandlers` (
  `eventhandler_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `eventhandler_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` varchar(255) NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`eventhandler_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical host and service event handlers';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_externalcommands`
-- 

CREATE TABLE `nagios_externalcommands` (
  `externalcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `command_type` smallint(6) NOT NULL default '0',
  `command_name` varchar(128) NOT NULL default '',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`externalcommand_id`)
) ENGINE=InnoDB COMMENT='Historical record of processed external commands';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_flappinghistory`
-- 

CREATE TABLE `nagios_flappinghistory` (
  `flappinghistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `event_time_usec` int(11) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `reason_type` smallint(6) NOT NULL default '0',
  `flapping_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `percent_state_change` double NOT NULL default '0',
  `low_threshold` double NOT NULL default '0',
  `high_threshold` double NOT NULL default '0',
  `comment_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `internal_comment_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`flappinghistory_id`)
) ENGINE=InnoDB COMMENT='Current and historical record of host and service flapping';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_host_contactgroups`
-- 

CREATE TABLE `nagios_host_contactgroups` (
  `host_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contactgroup_id`),
  UNIQUE KEY `instance_id` (`host_id`,`contactgroup_object_id`)
) ENGINE=InnoDB COMMENT='Host contact groups';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_host_contacts`
-- 

CREATE TABLE `nagios_host_contacts` (
  `host_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`host_id`,`contact_object_id`)
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_host_parenthosts`
-- 

CREATE TABLE `nagios_host_parenthosts` (
  `host_parenthost_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `parent_host_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_parenthost_id`),
  UNIQUE KEY `instance_id` (`host_id`,`parent_host_object_id`)
) ENGINE=InnoDB COMMENT='Parent hosts';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostchecks`
-- 

CREATE TABLE `nagios_hostchecks` (
  `hostcheck_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `check_type` smallint(6) NOT NULL default '0',
  `is_raw_check` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` varchar(255) NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `perfdata` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`hostcheck_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`host_object_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical host checks';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostdependencies`
-- 

CREATE TABLE `nagios_hostdependencies` (
  `hostdependency_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `dependent_host_object_id` int(11) NOT NULL default '0',
  `dependency_type` smallint(6) NOT NULL default '0',
  `inherits_parent` smallint(6) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `fail_on_up` smallint(6) NOT NULL default '0',
  `fail_on_down` smallint(6) NOT NULL default '0',
  `fail_on_unreachable` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`hostdependency_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`host_object_id`,`dependent_host_object_id`,`dependency_type`,`inherits_parent`,`fail_on_up`,`fail_on_down`,`fail_on_unreachable`)
) ENGINE=InnoDB COMMENT='Host dependency definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostescalation_contactgroups`
-- 

CREATE TABLE `nagios_hostescalation_contactgroups` (
  `hostescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`hostescalation_id`,`contactgroup_object_id`)
) ENGINE=InnoDB COMMENT='Host escalation contact groups';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostescalation_contacts`
-- 

CREATE TABLE `nagios_hostescalation_contacts` (
  `hostescalation_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`hostescalation_id`,`contact_object_id`)
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostescalations`
-- 

CREATE TABLE `nagios_hostescalations` (
  `hostescalation_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `first_notification` smallint(6) NOT NULL default '0',
  `last_notification` smallint(6) NOT NULL default '0',
  `notification_interval` double NOT NULL default '0',
  `escalate_on_recovery` smallint(6) NOT NULL default '0',
  `escalate_on_down` smallint(6) NOT NULL default '0',
  `escalate_on_unreachable` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`host_object_id`,`timeperiod_object_id`,`first_notification`,`last_notification`)
) ENGINE=InnoDB COMMENT='Host escalation definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostgroup_members`
-- 

CREATE TABLE `nagios_hostgroup_members` (
  `hostgroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostgroup_id` int(11) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostgroup_member_id`),
  UNIQUE KEY `instance_id` (`hostgroup_id`,`host_object_id`)
) ENGINE=InnoDB COMMENT='Hostgroup members';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hostgroups`
-- 

CREATE TABLE `nagios_hostgroups` (
  `hostgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `hostgroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`hostgroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`hostgroup_object_id`)
) ENGINE=InnoDB COMMENT='Hostgroup definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hosts`
-- 

CREATE TABLE `nagios_hosts` (
  `host_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `alias` varchar(64) NOT NULL default '',
  `display_name` varchar(64) NOT NULL default '',
  `address` varchar(128) NOT NULL default '',
  `check_command_object_id` int(11) NOT NULL default '0',
  `check_command_args` varchar(255) NOT NULL default '',
  `eventhandler_command_object_id` int(11) NOT NULL default '0',
  `eventhandler_command_args` varchar(255) NOT NULL default '',
  `notification_timeperiod_object_id` int(11) NOT NULL default '0',
  `check_timeperiod_object_id` int(11) NOT NULL default '0',
  `failure_prediction_options` varchar(64) NOT NULL default '',
  `check_interval` double NOT NULL default '0',
  `retry_interval` double NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `first_notification_delay` double NOT NULL default '0',
  `notification_interval` double NOT NULL default '0',
  `notify_on_down` smallint(6) NOT NULL default '0',
  `notify_on_unreachable` smallint(6) NOT NULL default '0',
  `notify_on_recovery` smallint(6) NOT NULL default '0',
  `notify_on_flapping` smallint(6) NOT NULL default '0',
  `notify_on_downtime` smallint(6) NOT NULL default '0',
  `stalk_on_up` smallint(6) NOT NULL default '0',
  `stalk_on_down` smallint(6) NOT NULL default '0',
  `stalk_on_unreachable` smallint(6) NOT NULL default '0',
  `flap_detection_enabled` smallint(6) NOT NULL default '0',
  `flap_detection_on_up` smallint(6) NOT NULL default '0',
  `flap_detection_on_down` smallint(6) NOT NULL default '0',
  `flap_detection_on_unreachable` smallint(6) NOT NULL default '0',
  `low_flap_threshold` double NOT NULL default '0',
  `high_flap_threshold` double NOT NULL default '0',
  `process_performance_data` smallint(6) NOT NULL default '0',
  `freshness_checks_enabled` smallint(6) NOT NULL default '0',
  `freshness_threshold` smallint(6) NOT NULL default '0',
  `passive_checks_enabled` smallint(6) NOT NULL default '0',
  `event_handler_enabled` smallint(6) NOT NULL default '0',
  `active_checks_enabled` smallint(6) NOT NULL default '0',
  `retain_status_information` smallint(6) NOT NULL default '0',
  `retain_nonstatus_information` smallint(6) NOT NULL default '0',
  `notifications_enabled` smallint(6) NOT NULL default '0',
  `obsess_over_host` smallint(6) NOT NULL default '0',
  `failure_prediction_enabled` smallint(6) NOT NULL default '0',
  `notes` varchar(255) NOT NULL default '',
  `notes_url` varchar(255) NOT NULL default '',
  `action_url` varchar(255) NOT NULL default '',
  `icon_image` varchar(255) NOT NULL default '',
  `icon_image_alt` varchar(255) NOT NULL default '',
  `vrml_image` varchar(255) NOT NULL default '',
  `statusmap_image` varchar(255) NOT NULL default '',
  `have_2d_coords` smallint(6) NOT NULL default '0',
  `x_2d` smallint(6) NOT NULL default '0',
  `y_2d` smallint(6) NOT NULL default '0',
  `have_3d_coords` smallint(6) NOT NULL default '0',
  `x_3d` double NOT NULL default '0',
  `y_3d` double NOT NULL default '0',
  `z_3d` double NOT NULL default '0',
  PRIMARY KEY  (`host_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`host_object_id`)
) ENGINE=InnoDB COMMENT='Host definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_hoststatus`
-- 

CREATE TABLE `nagios_hoststatus` (
  `hoststatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `output` varchar(255) NOT NULL default '',
  `perfdata` varchar(255) NOT NULL default '',
  `current_state` smallint(6) NOT NULL default '0',
  `has_been_checked` smallint(6) NOT NULL default '0',
  `should_be_scheduled` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `last_check` datetime NOT NULL default '0000-00-00 00:00:00',
  `next_check` datetime NOT NULL default '0000-00-00 00:00:00',
  `check_type` smallint(6) NOT NULL default '0',
  `last_state_change` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_hard_state_change` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_hard_state` smallint(6) NOT NULL default '0',
  `last_time_up` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_time_down` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_time_unreachable` datetime NOT NULL default '0000-00-00 00:00:00',
  `state_type` smallint(6) NOT NULL default '0',
  `last_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `next_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `no_more_notifications` smallint(6) NOT NULL default '0',
  `notifications_enabled` smallint(6) NOT NULL default '0',
  `problem_has_been_acknowledged` smallint(6) NOT NULL default '0',
  `acknowledgement_type` smallint(6) NOT NULL default '0',
  `current_notification_number` smallint(6) NOT NULL default '0',
  `passive_checks_enabled` smallint(6) NOT NULL default '0',
  `active_checks_enabled` smallint(6) NOT NULL default '0',
  `event_handler_enabled` smallint(6) NOT NULL default '0',
  `flap_detection_enabled` smallint(6) NOT NULL default '0',
  `is_flapping` smallint(6) NOT NULL default '0',
  `percent_state_change` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `scheduled_downtime_depth` smallint(6) NOT NULL default '0',
  `failure_prediction_enabled` smallint(6) NOT NULL default '0',
  `process_performance_data` smallint(6) NOT NULL default '0',
  `obsess_over_host` smallint(6) NOT NULL default '0',
  `modified_host_attributes` int(11) NOT NULL default '0',
  `event_handler` varchar(255) NOT NULL default '',
  `check_command` varchar(255) NOT NULL default '',
  `normal_check_interval` double NOT NULL default '0',
  `retry_check_interval` double NOT NULL default '0',
  `check_timeperiod_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hoststatus_id`),
  UNIQUE KEY `object_id` (`host_object_id`)
) ENGINE=InnoDB COMMENT='Current host status information';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_instances`
-- 

CREATE TABLE `nagios_instances` (
  `instance_id` smallint(6) NOT NULL auto_increment,
  `instance_name` varchar(64) NOT NULL default '',
  `instance_description` varchar(128) NOT NULL default '',
  PRIMARY KEY  (`instance_id`)
) ENGINE=InnoDB COMMENT='Location names of various Nagios installations';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_logentries`
-- 

CREATE TABLE `nagios_logentries` (
  `logentry_id` int(11) NOT NULL auto_increment,
  `instance_id` int(11) NOT NULL default '0',
  `logentry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `entry_time_usec` int(11) NOT NULL default '0',
  `logentry_type` int(11) NOT NULL default '0',
  `logentry_data` varchar(255) NOT NULL default '',
  `realtime_data` smallint(6) NOT NULL default '0',
  `inferred_data_extracted` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`logentry_id`)
) ENGINE=InnoDB COMMENT='Historical record of log entries';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_notifications`
-- 

CREATE TABLE `nagios_notifications` (
  `notification_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `notification_type` smallint(6) NOT NULL default '0',
  `notification_reason` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `escalated` smallint(6) NOT NULL default '0',
  `contacts_notified` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`notification_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical record of host and service notifications';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_objects`
-- 

CREATE TABLE `nagios_objects` (
  `object_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `objecttype_id` smallint(6) NOT NULL default '0',
  `name1` varchar(128) NOT NULL default '',
  `name2` varchar(128) default NULL,
  `is_active` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`object_id`),
  KEY `objecttype_id` (`objecttype_id`,`name1`,`name2`)
) ENGINE=InnoDB COMMENT='Current and historical objects of all kinds';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_processevents`
-- 

CREATE TABLE `nagios_processevents` (
  `processevent_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `event_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `event_time_usec` int(11) NOT NULL default '0',
  `process_id` int(11) NOT NULL default '0',
  `program_name` varchar(16) NOT NULL default '',
  `program_version` varchar(20) NOT NULL default '',
  `program_date` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`processevent_id`)
) ENGINE=InnoDB COMMENT='Historical Nagios process events';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_programstatus`
-- 

CREATE TABLE `nagios_programstatus` (
  `programstatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `program_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `program_end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `is_currently_running` smallint(6) NOT NULL default '0',
  `process_id` int(11) NOT NULL default '0',
  `daemon_mode` smallint(6) NOT NULL default '0',
  `last_command_check` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_log_rotation` datetime NOT NULL default '0000-00-00 00:00:00',
  `notifications_enabled` smallint(6) NOT NULL default '0',
  `active_service_checks_enabled` smallint(6) NOT NULL default '0',
  `passive_service_checks_enabled` smallint(6) NOT NULL default '0',
  `active_host_checks_enabled` smallint(6) NOT NULL default '0',
  `passive_host_checks_enabled` smallint(6) NOT NULL default '0',
  `event_handlers_enabled` smallint(6) NOT NULL default '0',
  `flap_detection_enabled` smallint(6) NOT NULL default '0',
  `failure_prediction_enabled` smallint(6) NOT NULL default '0',
  `process_performance_data` smallint(6) NOT NULL default '0',
  `obsess_over_hosts` smallint(6) NOT NULL default '0',
  `obsess_over_services` smallint(6) NOT NULL default '0',
  `modified_host_attributes` int(11) NOT NULL default '0',
  `modified_service_attributes` int(11) NOT NULL default '0',
  `global_host_event_handler` varchar(255) NOT NULL default '',
  `global_service_event_handler` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`programstatus_id`),
  UNIQUE KEY `instance_id` (`instance_id`)
) ENGINE=InnoDB COMMENT='Current program status information';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_runtimevariables`
-- 

CREATE TABLE `nagios_runtimevariables` (
  `runtimevariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `varname` varchar(64) NOT NULL default '',
  `varvalue` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`runtimevariable_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`varname`)
) ENGINE=InnoDB COMMENT='Runtime variables from the Nagios daemon';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_scheduleddowntime`
-- 

CREATE TABLE `nagios_scheduleddowntime` (
  `scheduleddowntime_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `downtime_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `internal_downtime_id` int(11) NOT NULL default '0',
  `triggered_by_id` int(11) NOT NULL default '0',
  `is_fixed` smallint(6) NOT NULL default '0',
  `duration` smallint(6) NOT NULL default '0',
  `scheduled_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `scheduled_end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `was_started` smallint(6) NOT NULL default '0',
  `actual_start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `actual_start_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`scheduleddowntime_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`entry_time`,`internal_downtime_id`)
) ENGINE=InnoDB COMMENT='Current scheduled host and service downtime';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_service_contactgroups`
-- 

CREATE TABLE `nagios_service_contactgroups` (
  `service_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contactgroup_id`),
  UNIQUE KEY `instance_id` (`service_id`,`contactgroup_object_id`)
) ENGINE=InnoDB COMMENT='Service contact groups';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_service_contacts`
-- 

CREATE TABLE `nagios_service_contacts` (
  `service_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`service_id`,`contact_object_id`)
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_servicechecks`
-- 

CREATE TABLE `nagios_servicechecks` (
  `servicecheck_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `check_type` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` varchar(255) NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `perfdata` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`servicecheck_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`service_object_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical service checks';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_servicedependencies`
-- 

CREATE TABLE `nagios_servicedependencies` (
  `servicedependency_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `dependent_service_object_id` int(11) NOT NULL default '0',
  `dependency_type` smallint(6) NOT NULL default '0',
  `inherits_parent` smallint(6) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `fail_on_ok` smallint(6) NOT NULL default '0',
  `fail_on_warning` smallint(6) NOT NULL default '0',
  `fail_on_unknown` smallint(6) NOT NULL default '0',
  `fail_on_critical` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`servicedependency_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`service_object_id`,`dependent_service_object_id`,`dependency_type`,`inherits_parent`,`fail_on_ok`,`fail_on_warning`,`fail_on_unknown`,`fail_on_critical`)
) ENGINE=InnoDB COMMENT='Service dependency definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_serviceescalation_contactgroups`
-- 

CREATE TABLE `nagios_serviceescalation_contactgroups` (
  `serviceescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`serviceescalation_id`,`contactgroup_object_id`)
) ENGINE=InnoDB COMMENT='Service escalation contact groups';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_serviceescalation_contacts`
-- 

CREATE TABLE `nagios_serviceescalation_contacts` (
  `serviceescalation_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`serviceescalation_id`,`contact_object_id`)
) ENGINE=InnoDB;

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_serviceescalations`
-- 

CREATE TABLE `nagios_serviceescalations` (
  `serviceescalation_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `first_notification` smallint(6) NOT NULL default '0',
  `last_notification` smallint(6) NOT NULL default '0',
  `notification_interval` double NOT NULL default '0',
  `escalate_on_recovery` smallint(6) NOT NULL default '0',
  `escalate_on_warning` smallint(6) NOT NULL default '0',
  `escalate_on_unknown` smallint(6) NOT NULL default '0',
  `escalate_on_critical` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`service_object_id`,`timeperiod_object_id`,`first_notification`,`last_notification`)
) ENGINE=InnoDB COMMENT='Service escalation definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_servicegroup_members`
-- 

CREATE TABLE `nagios_servicegroup_members` (
  `servicegroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `servicegroup_id` int(11) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`servicegroup_member_id`),
  UNIQUE KEY `instance_id` (`servicegroup_id`,`service_object_id`)
) ENGINE=InnoDB COMMENT='Servicegroup members';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_servicegroups`
-- 

CREATE TABLE `nagios_servicegroups` (
  `servicegroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `servicegroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`servicegroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`servicegroup_object_id`)
) ENGINE=InnoDB COMMENT='Servicegroup definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_services`
-- 

CREATE TABLE `nagios_services` (
  `service_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `display_name` varchar(64) NOT NULL default '',
  `check_command_object_id` int(11) NOT NULL default '0',
  `check_command_args` varchar(255) NOT NULL default '',
  `eventhandler_command_object_id` int(11) NOT NULL default '0',
  `eventhandler_command_args` varchar(255) NOT NULL default '',
  `notification_timeperiod_object_id` int(11) NOT NULL default '0',
  `check_timeperiod_object_id` int(11) NOT NULL default '0',
  `failure_prediction_options` varchar(64) NOT NULL default '',
  `check_interval` double NOT NULL default '0',
  `retry_interval` double NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `first_notification_delay` double NOT NULL default '0',
  `notification_interval` double NOT NULL default '0',
  `notify_on_warning` smallint(6) NOT NULL default '0',
  `notify_on_unknown` smallint(6) NOT NULL default '0',
  `notify_on_critical` smallint(6) NOT NULL default '0',
  `notify_on_recovery` smallint(6) NOT NULL default '0',
  `notify_on_flapping` smallint(6) NOT NULL default '0',
  `notify_on_downtime` smallint(6) NOT NULL default '0',
  `stalk_on_ok` smallint(6) NOT NULL default '0',
  `stalk_on_warning` smallint(6) NOT NULL default '0',
  `stalk_on_unknown` smallint(6) NOT NULL default '0',
  `stalk_on_critical` smallint(6) NOT NULL default '0',
  `is_volatile` smallint(6) NOT NULL default '0',
  `flap_detection_enabled` smallint(6) NOT NULL default '0',
  `flap_detection_on_ok` smallint(6) NOT NULL default '0',
  `flap_detection_on_warning` smallint(6) NOT NULL default '0',
  `flap_detection_on_unknown` smallint(6) NOT NULL default '0',
  `flap_detection_on_critical` smallint(6) NOT NULL default '0',
  `low_flap_threshold` double NOT NULL default '0',
  `high_flap_threshold` double NOT NULL default '0',
  `process_performance_data` smallint(6) NOT NULL default '0',
  `freshness_checks_enabled` smallint(6) NOT NULL default '0',
  `freshness_threshold` smallint(6) NOT NULL default '0',
  `passive_checks_enabled` smallint(6) NOT NULL default '0',
  `event_handler_enabled` smallint(6) NOT NULL default '0',
  `active_checks_enabled` smallint(6) NOT NULL default '0',
  `retain_status_information` smallint(6) NOT NULL default '0',
  `retain_nonstatus_information` smallint(6) NOT NULL default '0',
  `notifications_enabled` smallint(6) NOT NULL default '0',
  `obsess_over_service` smallint(6) NOT NULL default '0',
  `failure_prediction_enabled` smallint(6) NOT NULL default '0',
  `notes` varchar(255) NOT NULL default '',
  `notes_url` varchar(255) NOT NULL default '',
  `action_url` varchar(255) NOT NULL default '',
  `icon_image` varchar(255) NOT NULL default '',
  `icon_image_alt` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`service_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`service_object_id`)
) ENGINE=InnoDB COMMENT='Service definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_servicestatus`
-- 

CREATE TABLE `nagios_servicestatus` (
  `servicestatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `output` varchar(255) NOT NULL default '',
  `perfdata` varchar(255) NOT NULL default '',
  `current_state` smallint(6) NOT NULL default '0',
  `has_been_checked` smallint(6) NOT NULL default '0',
  `should_be_scheduled` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `last_check` datetime NOT NULL default '0000-00-00 00:00:00',
  `next_check` datetime NOT NULL default '0000-00-00 00:00:00',
  `check_type` smallint(6) NOT NULL default '0',
  `last_state_change` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_hard_state_change` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_hard_state` smallint(6) NOT NULL default '0',
  `last_time_ok` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_time_warning` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_time_unknown` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_time_critical` datetime NOT NULL default '0000-00-00 00:00:00',
  `state_type` smallint(6) NOT NULL default '0',
  `last_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `next_notification` datetime NOT NULL default '0000-00-00 00:00:00',
  `no_more_notifications` smallint(6) NOT NULL default '0',
  `notifications_enabled` smallint(6) NOT NULL default '0',
  `problem_has_been_acknowledged` smallint(6) NOT NULL default '0',
  `acknowledgement_type` smallint(6) NOT NULL default '0',
  `current_notification_number` smallint(6) NOT NULL default '0',
  `passive_checks_enabled` smallint(6) NOT NULL default '0',
  `active_checks_enabled` smallint(6) NOT NULL default '0',
  `event_handler_enabled` smallint(6) NOT NULL default '0',
  `flap_detection_enabled` smallint(6) NOT NULL default '0',
  `is_flapping` smallint(6) NOT NULL default '0',
  `percent_state_change` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `scheduled_downtime_depth` smallint(6) NOT NULL default '0',
  `failure_prediction_enabled` smallint(6) NOT NULL default '0',
  `process_performance_data` smallint(6) NOT NULL default '0',
  `obsess_over_service` smallint(6) NOT NULL default '0',
  `modified_service_attributes` int(11) NOT NULL default '0',
  `event_handler` varchar(255) NOT NULL default '',
  `check_command` varchar(255) NOT NULL default '',
  `normal_check_interval` double NOT NULL default '0',
  `retry_check_interval` double NOT NULL default '0',
  `check_timeperiod_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`servicestatus_id`),
  UNIQUE KEY `object_id` (`service_object_id`)
) ENGINE=InnoDB COMMENT='Current service status information';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_statehistory`
-- 

CREATE TABLE `nagios_statehistory` (
  `statehistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `state_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `state_time_usec` int(11) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `state_change` smallint(6) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `last_state` smallint(6) NOT NULL default '-1',
  `last_hard_state` smallint(6) NOT NULL default '-1',
  `output` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`statehistory_id`)
) ENGINE=InnoDB COMMENT='Historical host and service state changes';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_systemcommands`
-- 

CREATE TABLE `nagios_systemcommands` (
  `systemcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_line` varchar(255) NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`systemcommand_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`start_time`,`start_time_usec`)
) ENGINE=InnoDB COMMENT='Historical system commands that are executed';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_timedeventqueue`
-- 

CREATE TABLE `nagios_timedeventqueue` (
  `timedeventqueue_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `queued_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `queued_time_usec` int(11) NOT NULL default '0',
  `scheduled_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `recurring_event` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timedeventqueue_id`)
) ENGINE=InnoDB COMMENT='Current Nagios event queue';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_timedevents`
-- 

CREATE TABLE `nagios_timedevents` (
  `timedevent_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `queued_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `queued_time_usec` int(11) NOT NULL default '0',
  `event_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `event_time_usec` int(11) NOT NULL default '0',
  `scheduled_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `recurring_event` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `deletion_time` datetime NOT NULL default '0000-00-00 00:00:00',
  `deletion_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timedevent_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`event_type`,`scheduled_time`,`object_id`)
) ENGINE=InnoDB COMMENT='Historical events from the Nagios event queue';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_timeperiod_timeranges`
-- 

CREATE TABLE `nagios_timeperiod_timeranges` (
  `timeperiod_timerange_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `timeperiod_id` int(11) NOT NULL default '0',
  `day` smallint(6) NOT NULL default '0',
  `start_sec` int(11) NOT NULL default '0',
  `end_sec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timeperiod_timerange_id`),
  UNIQUE KEY `instance_id` (`timeperiod_id`,`day`,`start_sec`,`end_sec`)
) ENGINE=InnoDB COMMENT='Timeperiod definitions';

-- --------------------------------------------------------

-- 
-- Table structure for table `nagios_timeperiods`
-- 

CREATE TABLE `nagios_timeperiods` (
  `timeperiod_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`timeperiod_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`timeperiod_object_id`)
) ENGINE=InnoDB COMMENT='Timeperiod definitions';
