
SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET NAMES utf8;


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;


CREATE TABLE IF NOT EXISTS `nagios_acknowledgements` (
  `acknowledgement_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `entry_time_usec` int(11) NOT NULL default '0',
  `acknowledgement_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_sticky` smallint(6) NOT NULL default '0',
  `persistent_comment` smallint(6) NOT NULL default '0',
  `notify_contacts` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`acknowledgement_id`),
  UNIQUE KEY `instance_id` (`instance_id`, `entry_time`, `entry_time_usec`)
) ENGINE=MyISAM COMMENT='Current and historical host and service acknowledgements';


CREATE TABLE IF NOT EXISTS `nagios_commands` (
  `command_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `command_line` TEXT NOT NULL default '',
  PRIMARY KEY  (`command_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`config_type`)
) ENGINE=MyISAM  COMMENT='Command definitions';


CREATE TABLE IF NOT EXISTS `nagios_commenthistory` (
  `commenthistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `entry_time_usec` int(11) NOT NULL default '0',
  `comment_type` smallint(6) NOT NULL default '0',
  `entry_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `comment_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `internal_comment_id` int(11) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_persistent` smallint(6) NOT NULL default '0',
  `comment_source` smallint(6) NOT NULL default '0',
  `expires` smallint(6) NOT NULL default '0',
  `expiration_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `deletion_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `deletion_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`commenthistory_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`comment_time`,`internal_comment_id`)
) ENGINE=MyISAM  COMMENT='Historical host and service comments';


CREATE TABLE IF NOT EXISTS `nagios_comments` (
  `comment_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `entry_time_usec` int(11) NOT NULL default '0',
  `comment_type` smallint(6) NOT NULL default '0',
  `entry_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `comment_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `internal_comment_id` int(11) NOT NULL default '0',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `is_persistent` smallint(6) NOT NULL default '0',
  `comment_source` smallint(6) NOT NULL default '0',
  `expires` smallint(6) NOT NULL default '0',
  `expiration_time` datetime NOT NULL default '1970-01-01 00:00:01',
  PRIMARY KEY  (`comment_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`comment_time`,`internal_comment_id`)
) ENGINE=MyISAM ;


CREATE TABLE IF NOT EXISTS `nagios_configfiles` (
  `configfile_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `configfile_type` smallint(6) NOT NULL default '0',
  `configfile_path` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`configfile_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`configfile_type`,`configfile_path`)
) ENGINE=MyISAM  COMMENT='Configuration files';


CREATE TABLE IF NOT EXISTS `nagios_configfilevariables` (
  `configfilevariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `configfile_id` int(11) NOT NULL default '0',
  `varname` varchar(64) NOT NULL default '',
  `varvalue` varchar(1024) NOT NULL default '',
  PRIMARY KEY  (`configfilevariable_id`)
) ENGINE=MyISAM  COMMENT='Configuration file variables';


CREATE TABLE IF NOT EXISTS `nagios_conninfo` (
  `conninfo_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `agent_name` varchar(32) NOT NULL default '',
  `agent_version` varchar(8) NOT NULL default '',
  `disposition` varchar(16) NOT NULL default '',
  `connect_source` varchar(16) NOT NULL default '',
  `connect_type` varchar(16) NOT NULL default '',
  `connect_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `disconnect_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_checkin_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `data_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `data_end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `bytes_processed` int(11) NOT NULL default '0',
  `lines_processed` int(11) NOT NULL default '0',
  `entries_processed` int(11) NOT NULL default '0',
  PRIMARY KEY  (`conninfo_id`)
) ENGINE=MyISAM  COMMENT='NDO2DB daemon connection information';


CREATE TABLE IF NOT EXISTS `nagios_contactgroups` (
  `contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contactgroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`contactgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Contactgroup definitions';


CREATE TABLE IF NOT EXISTS `nagios_contactgroup_members` (
  `contactgroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contactgroup_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactgroup_member_id`),
  UNIQUE KEY `instance_id` (`contactgroup_id`,`contact_object_id`)
) ENGINE=MyISAM  COMMENT='Contactgroup members';


CREATE TABLE IF NOT EXISTS `nagios_contactnotificationmethods` (
  `contactnotificationmethod_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contactnotification_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contactnotificationmethod_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`contactnotification_id`,`start_time`,`start_time_usec`)
) ENGINE=MyISAM  COMMENT='Historical record of contact notification methods';


CREATE TABLE IF NOT EXISTS `nagios_contactnotifications` (
  `contactnotification_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `notification_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactnotification_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`contact_object_id`,`start_time`,`start_time_usec`)
) ENGINE=MyISAM  COMMENT='Historical record of contact notifications';


CREATE TABLE IF NOT EXISTS `nagios_contacts` (
  `contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `alias` varchar(64) NOT NULL default '',
  `email_address` varchar(255) NOT NULL default '',
  `pager_address` varchar(64) NOT NULL default '',
  `minimum_importance` int(11) NOT NULL default '0',
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
) ENGINE=MyISAM  COMMENT='Contact definitions';


CREATE TABLE IF NOT EXISTS `nagios_contactstatus` (
  `contactstatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `host_notifications_enabled` smallint(6) NOT NULL default '0',
  `service_notifications_enabled` smallint(6) NOT NULL default '0',
  `last_host_notification` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_service_notification` datetime NOT NULL default '1970-01-01 00:00:01',
  `modified_attributes` int(11) NOT NULL default '0',
  `modified_host_attributes` int(11) NOT NULL default '0',
  `modified_service_attributes` int(11) NOT NULL default '0',
  PRIMARY KEY  (`contactstatus_id`),
  UNIQUE KEY `contact_object_id` (`contact_object_id`)
) ENGINE=MyISAM  COMMENT='Contact status';


CREATE TABLE IF NOT EXISTS `nagios_contact_addresses` (
  `contact_address_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_id` int(11) NOT NULL default '0',
  `address_number` smallint(6) NOT NULL default '0',
  `address` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contact_address_id`),
  UNIQUE KEY `contact_id` (`contact_id`,`address_number`)
) ENGINE=MyISAM COMMENT='Contact addresses';


CREATE TABLE IF NOT EXISTS `nagios_contact_notificationcommands` (
  `contact_notificationcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `contact_id` int(11) NOT NULL default '0',
  `notification_type` smallint(6) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`contact_notificationcommand_id`),
  UNIQUE KEY `contact_id` (`contact_id`,`notification_type`,`command_object_id`,`command_args`)
) ENGINE=MyISAM  COMMENT='Contact host and service notification commands';


CREATE TABLE IF NOT EXISTS `nagios_customvariables` (
  `customvariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `has_been_modified` smallint(6) NOT NULL default '0',
  `varname` varchar(255) NOT NULL default '',
  `varvalue` varchar(1024) NOT NULL default '',
  PRIMARY KEY  (`customvariable_id`),
  UNIQUE KEY `object_id_2` (`object_id`,`config_type`,`varname`),
  KEY `varname` (`varname`)
) ENGINE=MyISAM COMMENT='Custom variables';


CREATE TABLE IF NOT EXISTS `nagios_customvariablestatus` (
  `customvariablestatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `has_been_modified` smallint(6) NOT NULL default '0',
  `varname` varchar(255) NOT NULL default '',
  `varvalue` varchar(1024) NOT NULL default '',
  PRIMARY KEY  (`customvariablestatus_id`),
  UNIQUE KEY `object_id_2` (`object_id`,`varname`),
  KEY `varname` (`varname`)
) ENGINE=MyISAM COMMENT='Custom variable status information';


CREATE TABLE IF NOT EXISTS `nagios_dbversion` (
  `name` varchar(10) NOT NULL default '',
  `version` varchar(10) NOT NULL default ''
) ENGINE=MyISAM;


CREATE TABLE IF NOT EXISTS `nagios_downtimehistory` (
  `downtimehistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `downtime_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `internal_downtime_id` int(11) NOT NULL default '0',
  `triggered_by_id` int(11) NOT NULL default '0',
  `is_fixed` smallint(6) NOT NULL default '0',
  `duration` smallint(6) NOT NULL default '0',
  `scheduled_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `scheduled_end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `was_started` smallint(6) NOT NULL default '0',
  `actual_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `actual_start_time_usec` int(11) NOT NULL default '0',
  `actual_end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `actual_end_time_usec` int(11) NOT NULL default '0',
  `was_cancelled` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`downtimehistory_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`entry_time`,`internal_downtime_id`)
) ENGINE=MyISAM  COMMENT='Historical scheduled host and service downtime';


CREATE TABLE IF NOT EXISTS `nagios_eventhandlers` (
  `eventhandler_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `eventhandler_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` TEXT NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  PRIMARY KEY  (`eventhandler_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`start_time`,`start_time_usec`)
) ENGINE=MyISAM COMMENT='Historical host and service event handlers';


CREATE TABLE IF NOT EXISTS `nagios_externalcommands` (
  `externalcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `command_type` smallint(6) NOT NULL default '0',
  `command_name` varchar(128) NOT NULL default '',
  `command_args` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`externalcommand_id`)
) ENGINE=MyISAM  COMMENT='Historical record of processed external commands';


CREATE TABLE IF NOT EXISTS `nagios_flappinghistory` (
  `flappinghistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `event_time_usec` int(11) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `reason_type` smallint(6) NOT NULL default '0',
  `flapping_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `percent_state_change` double NOT NULL default '0',
  `low_threshold` double NOT NULL default '0',
  `high_threshold` double NOT NULL default '0',
  `comment_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `internal_comment_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`flappinghistory_id`)
) ENGINE=MyISAM  COMMENT='Current and historical record of host and service flapping';


CREATE TABLE IF NOT EXISTS `nagios_hostchecks` (
  `hostcheck_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `check_type` smallint(6) NOT NULL default '0',
  `is_raw_check` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` TEXT NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  `perfdata` TEXT NOT NULL default '',
  PRIMARY KEY  (`hostcheck_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`host_object_id`,`start_time`,`start_time_usec`)
) ENGINE=MyISAM  COMMENT='Historical host checks';


CREATE TABLE IF NOT EXISTS `nagios_hostdependencies` (
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
) ENGINE=MyISAM COMMENT='Host dependency definitions';


CREATE TABLE IF NOT EXISTS `nagios_hostescalations` (
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
) ENGINE=MyISAM  COMMENT='Host escalation definitions';


CREATE TABLE IF NOT EXISTS `nagios_hostescalation_contactgroups` (
  `hostescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`hostescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Host escalation contact groups';


CREATE TABLE IF NOT EXISTS `nagios_hostescalation_contacts` (
  `hostescalation_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`hostescalation_id`,`contact_object_id`)
) ENGINE=MyISAM ;


CREATE TABLE IF NOT EXISTS `nagios_hostgroups` (
  `hostgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `hostgroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`hostgroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`hostgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Hostgroup definitions';


CREATE TABLE IF NOT EXISTS `nagios_hostgroup_members` (
  `hostgroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostgroup_id` int(11) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostgroup_member_id`),
  UNIQUE KEY `instance_id` (`hostgroup_id`,`host_object_id`)
) ENGINE=MyISAM  COMMENT='Hostgroup members';


CREATE TABLE IF NOT EXISTS `nagios_hosts` (
  `host_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `alias` varchar(64) NOT NULL default '',
  `display_name` varchar(64) NOT NULL default '',
  `address` varchar(128) NOT NULL default '',
  `importance` int(11) NOT NULL default '0',
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
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`host_object_id`),
  KEY `host_object_id` (`host_object_id`)
) ENGINE=MyISAM  COMMENT='Host definitions';


CREATE TABLE IF NOT EXISTS `nagios_hoststatus` (
  `hoststatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  `perfdata` TEXT NOT NULL default '',
  `current_state` smallint(6) NOT NULL default '0',
  `has_been_checked` smallint(6) NOT NULL default '0',
  `should_be_scheduled` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `last_check` datetime NOT NULL default '1970-01-01 00:00:01',
  `next_check` datetime NOT NULL default '1970-01-01 00:00:01',
  `check_type` smallint(6) NOT NULL default '0',
  `check_options` smallint(6) NOT NULL default '0',
  `last_state_change` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_hard_state_change` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_hard_state` smallint(6) NOT NULL default '0',
  `last_time_up` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_time_down` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_time_unreachable` datetime NOT NULL default '1970-01-01 00:00:01',
  `state_type` smallint(6) NOT NULL default '0',
  `last_notification` datetime NOT NULL default '1970-01-01 00:00:01',
  `next_notification` datetime NOT NULL default '1970-01-01 00:00:01',
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
  UNIQUE KEY `object_id` (`host_object_id`),
  KEY `instance_id` (`instance_id`),
  KEY `status_update_time` (`status_update_time`),
  KEY `current_state` (`current_state`),
  KEY `check_type` (`check_type`),
  KEY `state_type` (`state_type`),
  KEY `last_state_change` (`last_state_change`),
  KEY `notifications_enabled` (`notifications_enabled`),
  KEY `problem_has_been_acknowledged` (`problem_has_been_acknowledged`),
  KEY `active_checks_enabled` (`active_checks_enabled`),
  KEY `passive_checks_enabled` (`passive_checks_enabled`),
  KEY `event_handler_enabled` (`event_handler_enabled`),
  KEY `flap_detection_enabled` (`flap_detection_enabled`),
  KEY `is_flapping` (`is_flapping`),
  KEY `percent_state_change` (`percent_state_change`),
  KEY `latency` (`latency`),
  KEY `execution_time` (`execution_time`),
  KEY `scheduled_downtime_depth` (`scheduled_downtime_depth`)
) ENGINE=MyISAM  COMMENT='Current host status information';


CREATE TABLE IF NOT EXISTS `nagios_host_contactgroups` (
  `host_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contactgroup_id`),
  UNIQUE KEY `instance_id` (`host_id`,`contactgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Host contact groups';


CREATE TABLE IF NOT EXISTS `nagios_host_contacts` (
  `host_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`host_id`,`contact_object_id`)
) ENGINE=MyISAM ;


CREATE TABLE IF NOT EXISTS `nagios_host_parenthosts` (
  `host_parenthost_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `parent_host_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_parenthost_id`),
  UNIQUE KEY `instance_id` (`host_id`,`parent_host_object_id`)
) ENGINE=MyISAM  COMMENT='Parent hosts';


CREATE TABLE IF NOT EXISTS `nagios_instances` (
  `instance_id` smallint(6) NOT NULL auto_increment,
  `instance_name` varchar(64) NOT NULL default '',
  `instance_description` varchar(128) NOT NULL default '',
  PRIMARY KEY  (`instance_id`)
) ENGINE=MyISAM  COMMENT='Location names of various Nagios installations';


CREATE TABLE IF NOT EXISTS `nagios_logentries` (
  `logentry_id` int(11) NOT NULL auto_increment,
  `instance_id` int(11) NOT NULL default '0',
  `logentry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `entry_time_usec` int(11) NOT NULL default '0',
  `logentry_type` int(11) NOT NULL default '0',
  `logentry_data` varchar(255) NOT NULL default '',
  `realtime_data` smallint(6) NOT NULL default '0',
  `inferred_data_extracted` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`logentry_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`logentry_time`,`entry_time`,`entry_time_usec`,`logentry_id`)
) ENGINE=MyISAM COMMENT='Historical record of log entries';


CREATE TABLE IF NOT EXISTS `nagios_notifications` (
  `notification_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `notification_type` smallint(6) NOT NULL default '0',
  `notification_reason` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  `escalated` smallint(6) NOT NULL default '0',
  `contacts_notified` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`notification_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`start_time`,`start_time_usec`)
) ENGINE=MyISAM  COMMENT='Historical record of host and service notifications';


CREATE TABLE IF NOT EXISTS `nagios_objects` (
  `object_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `objecttype_id` smallint(6) NOT NULL default '0',
  `name1` varchar(162) NOT NULL default '',
  `name2` varchar(162) NOT NULL default '',
  `is_active` smallint(6) NOT NULL default '0',
  PRIMARY KEY  (`object_id`),
  UNIQUE KEY `uniq_object` (`objecttype_id`,`name1`,`name2`)
) ENGINE=MyISAM  COMMENT='Current and historical objects of all kinds';


CREATE TABLE IF NOT EXISTS `nagios_processevents` (
  `processevent_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `event_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `event_time_usec` int(11) NOT NULL default '0',
  `process_id` int(11) NOT NULL default '0',
  `program_name` varchar(16) NOT NULL default '',
  `program_version` varchar(20) NOT NULL default '',
  `program_date` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`processevent_id`)
) ENGINE=MyISAM  COMMENT='Historical Nagios process events';


CREATE TABLE IF NOT EXISTS `nagios_programstatus` (
  `programstatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `program_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `program_end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `is_currently_running` smallint(6) NOT NULL default '0',
  `process_id` int(11) NOT NULL default '0',
  `daemon_mode` smallint(6) NOT NULL default '0',
  `last_command_check` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_log_rotation` datetime NOT NULL default '1970-01-01 00:00:01',
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
) ENGINE=MyISAM  COMMENT='Current program status information';


CREATE TABLE IF NOT EXISTS `nagios_runtimevariables` (
  `runtimevariable_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `varname` varchar(64) NOT NULL default '',
  `varvalue` varchar(1024) NOT NULL default '',
  PRIMARY KEY  (`runtimevariable_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`varname`)
) ENGINE=MyISAM  COMMENT='Runtime variables from the Nagios daemon';


CREATE TABLE IF NOT EXISTS `nagios_scheduleddowntime` (
  `scheduleddowntime_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `downtime_type` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `entry_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `author_name` varchar(64) NOT NULL default '',
  `comment_data` varchar(255) NOT NULL default '',
  `internal_downtime_id` int(11) NOT NULL default '0',
  `triggered_by_id` int(11) NOT NULL default '0',
  `is_fixed` smallint(6) NOT NULL default '0',
  `duration` int NOT NULL default '0',
  `scheduled_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `scheduled_end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `was_started` smallint(6) NOT NULL default '0',
  `actual_start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `actual_start_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`scheduleddowntime_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`object_id`,`entry_time`,`internal_downtime_id`)
) ENGINE=MyISAM COMMENT='Current scheduled host and service downtime';


CREATE TABLE IF NOT EXISTS `nagios_servicechecks` (
  `servicecheck_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `check_type` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `state` smallint(6) NOT NULL default '0',
  `state_type` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_object_id` int(11) NOT NULL default '0',
  `command_args` varchar(255) NOT NULL default '',
  `command_line` TEXT NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `latency` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  `perfdata` TEXT NOT NULL default '',
  PRIMARY KEY  (`servicecheck_id`),
  KEY `instance_id` (`instance_id`),
  KEY `service_object_id` (`service_object_id`),
  KEY `start_time` (`start_time`)
) ENGINE=MyISAM  COMMENT='Historical service checks';


CREATE TABLE IF NOT EXISTS `nagios_servicedependencies` (
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
) ENGINE=MyISAM COMMENT='Service dependency definitions';


CREATE TABLE IF NOT EXISTS `nagios_serviceescalations` (
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
) ENGINE=MyISAM  COMMENT='Service escalation definitions';


CREATE TABLE IF NOT EXISTS `nagios_serviceescalation_contactgroups` (
  `serviceescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`serviceescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Service escalation contact groups';


CREATE TABLE IF NOT EXISTS `nagios_serviceescalation_contacts` (
  `serviceescalation_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`serviceescalation_id`,`contact_object_id`)
) ENGINE=MyISAM ;


CREATE TABLE IF NOT EXISTS `nagios_servicegroups` (
  `servicegroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `servicegroup_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`servicegroup_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`servicegroup_object_id`)
) ENGINE=MyISAM  COMMENT='Servicegroup definitions';


CREATE TABLE IF NOT EXISTS `nagios_servicegroup_members` (
  `servicegroup_member_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `servicegroup_id` int(11) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`servicegroup_member_id`),
  UNIQUE KEY `instance_id` (`servicegroup_id`,`service_object_id`)
) ENGINE=MyISAM  COMMENT='Servicegroup members';


CREATE TABLE IF NOT EXISTS `nagios_services` (
  `service_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `host_object_id` int(11) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `display_name` varchar(64) NOT NULL default '',
  `importance` int(11) NOT NULL default '0',
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
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`service_object_id`),
  KEY `service_object_id` (`service_object_id`)
) ENGINE=MyISAM  COMMENT='Service definitions';


CREATE TABLE IF NOT EXISTS `nagios_servicestatus` (
  `servicestatus_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_object_id` int(11) NOT NULL default '0',
  `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  `perfdata` TEXT NOT NULL default '',
  `current_state` smallint(6) NOT NULL default '0',
  `has_been_checked` smallint(6) NOT NULL default '0',
  `should_be_scheduled` smallint(6) NOT NULL default '0',
  `current_check_attempt` smallint(6) NOT NULL default '0',
  `max_check_attempts` smallint(6) NOT NULL default '0',
  `last_check` datetime NOT NULL default '1970-01-01 00:00:01',
  `next_check` datetime NOT NULL default '1970-01-01 00:00:01',
  `check_type` smallint(6) NOT NULL default '0',
  `check_options` smallint(6) NOT NULL default '0',
  `last_state_change` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_hard_state_change` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_hard_state` smallint(6) NOT NULL default '0',
  `last_time_ok` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_time_warning` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_time_unknown` datetime NOT NULL default '1970-01-01 00:00:01',
  `last_time_critical` datetime NOT NULL default '1970-01-01 00:00:01',
  `state_type` smallint(6) NOT NULL default '0',
  `last_notification` datetime NOT NULL default '1970-01-01 00:00:01',
  `next_notification` datetime NOT NULL default '1970-01-01 00:00:01',
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
  UNIQUE KEY `object_id` (`service_object_id`),
  KEY `instance_id` (`instance_id`),
  KEY `status_update_time` (`status_update_time`),
  KEY `current_state` (`current_state`),
  KEY `check_type` (`check_type`),
  KEY `state_type` (`state_type`),
  KEY `last_state_change` (`last_state_change`),
  KEY `notifications_enabled` (`notifications_enabled`),
  KEY `problem_has_been_acknowledged` (`problem_has_been_acknowledged`),
  KEY `active_checks_enabled` (`active_checks_enabled`),
  KEY `passive_checks_enabled` (`passive_checks_enabled`),
  KEY `event_handler_enabled` (`event_handler_enabled`),
  KEY `flap_detection_enabled` (`flap_detection_enabled`),
  KEY `is_flapping` (`is_flapping`),
  KEY `percent_state_change` (`percent_state_change`),
  KEY `latency` (`latency`),
  KEY `execution_time` (`execution_time`),
  KEY `scheduled_downtime_depth` (`scheduled_downtime_depth`)
) ENGINE=MyISAM  COMMENT='Current service status information';


CREATE TABLE IF NOT EXISTS `nagios_service_contactgroups` (
  `service_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contactgroup_id`),
  UNIQUE KEY `instance_id` (`service_id`,`contactgroup_object_id`)
) ENGINE=MyISAM  COMMENT='Service contact groups';


CREATE TABLE IF NOT EXISTS `nagios_service_contacts` (
  `service_contact_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contact_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contact_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`service_id`,`contact_object_id`)
) ENGINE=MyISAM ;


CREATE TABLE IF NOT EXISTS `nagios_service_parentservices` (
  `service_parentservice_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `parent_service_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_parentservice_id`),
  UNIQUE KEY `instance_id` (`service_id`,`parent_service_object_id`)
) ENGINE=MyISAM  COMMENT='Parent services';


CREATE TABLE IF NOT EXISTS `nagios_statehistory` (
  `statehistory_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `state_time` datetime NOT NULL default '1970-01-01 00:00:01',
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
  `long_output` TEXT NOT NULL default '',
  PRIMARY KEY  (`statehistory_id`)
) ENGINE=MyISAM COMMENT='Historical host and service state changes';


CREATE TABLE IF NOT EXISTS `nagios_systemcommands` (
  `systemcommand_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `start_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `start_time_usec` int(11) NOT NULL default '0',
  `end_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `end_time_usec` int(11) NOT NULL default '0',
  `command_line` TEXT NOT NULL default '',
  `timeout` smallint(6) NOT NULL default '0',
  `early_timeout` smallint(6) NOT NULL default '0',
  `execution_time` double NOT NULL default '0',
  `return_code` smallint(6) NOT NULL default '0',
  `output` varchar(255) NOT NULL default '',
  `long_output` TEXT NOT NULL default '',
  PRIMARY KEY  (`systemcommand_id`),
  KEY `instance_id` (`instance_id`),
  KEY `start_time` (`start_time`)
) ENGINE=MyISAM  COMMENT='Historical system commands that are executed';


CREATE TABLE IF NOT EXISTS `nagios_timedeventqueue` (
  `timedeventqueue_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `queued_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `queued_time_usec` int(11) NOT NULL default '0',
  `scheduled_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `recurring_event` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timedeventqueue_id`),
  KEY `instance_id` (`instance_id`),
  KEY `event_type` (`event_type`),
  KEY `scheduled_time` (`scheduled_time`),
  KEY `object_id` (`object_id`)
) ENGINE=MyISAM  COMMENT='Current Nagios event queue';


CREATE TABLE IF NOT EXISTS `nagios_timedevents` (
  `timedevent_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `event_type` smallint(6) NOT NULL default '0',
  `queued_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `queued_time_usec` int(11) NOT NULL default '0',
  `event_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `event_time_usec` int(11) NOT NULL default '0',
  `scheduled_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `recurring_event` smallint(6) NOT NULL default '0',
  `object_id` int(11) NOT NULL default '0',
  `deletion_time` datetime NOT NULL default '1970-01-01 00:00:01',
  `deletion_time_usec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timedevent_id`),
  KEY `instance_id` (`instance_id`),
  KEY `event_type` (`event_type`),
  KEY `scheduled_time` (`scheduled_time`),
  KEY `object_id` (`object_id`)
) ENGINE=MyISAM  COMMENT='Historical events from the Nagios event queue';


CREATE TABLE IF NOT EXISTS `nagios_timeperiods` (
  `timeperiod_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `config_type` smallint(6) NOT NULL default '0',
  `timeperiod_object_id` int(11) NOT NULL default '0',
  `alias` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`timeperiod_id`),
  UNIQUE KEY `instance_id` (`instance_id`,`config_type`,`timeperiod_object_id`)
) ENGINE=MyISAM  COMMENT='Timeperiod definitions';


CREATE TABLE IF NOT EXISTS `nagios_timeperiod_timeranges` (
  `timeperiod_timerange_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `timeperiod_id` int(11) NOT NULL default '0',
  `day` smallint(6) NOT NULL default '0',
  `start_sec` int(11) NOT NULL default '0',
  `end_sec` int(11) NOT NULL default '0',
  PRIMARY KEY  (`timeperiod_timerange_id`),
  UNIQUE KEY `instance_id` (`timeperiod_id`,`day`,`start_sec`,`end_sec`)
) ENGINE=MyISAM  COMMENT='Timeperiod definitions';
