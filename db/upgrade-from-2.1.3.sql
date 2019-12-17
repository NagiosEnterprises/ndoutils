SET NAMES utf8;

ALTER TABLE `nagios_acknowledgements` MODIFY `entry_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_acknowledgements` MODIFY `author_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_acknowledgements` MODIFY `comment_data` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_commands` MODIFY `command_line` TEXT NOT NULL default '';

ALTER TABLE `nagios_commenthistory` MODIFY `entry_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_commenthistory` MODIFY `comment_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_commenthistory` MODIFY `expiration_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_commenthistory` MODIFY `deletion_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_commenthistory` MODIFY `author_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_commenthistory` MODIFY `comment_data` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_comments` MODIFY `entry_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_comments` MODIFY `comment_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_comments` MODIFY `expiration_time` datetime NOT NULL DEFAULT '1970-01-01 00:00:01';
ALTER TABLE `nagios_comments` MODIFY `author_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_comments` MODIFY `comment_data` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_configfiles` MODIFY `configfile_path` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_configfilevariables` MODIFY `varname` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_configfilevariables` MODIFY `varvalue` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_conninfo` MODIFY `agent_name` varchar(32) NOT NULL default '';
ALTER TABLE `nagios_conninfo` MODIFY `agent_version` varchar(8) NOT NULL default '';
ALTER TABLE `nagios_conninfo` MODIFY `disposition` varchar(16) NOT NULL default '';
ALTER TABLE `nagios_conninfo` MODIFY `connect_source` varchar(16) NOT NULL default '';
ALTER TABLE `nagios_conninfo` MODIFY `connect_type` varchar(16) NOT NULL default '';
ALTER TABLE `nagios_conninfo` MODIFY `connect_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_conninfo` MODIFY `disconnect_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_conninfo` MODIFY `last_checkin_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_conninfo` MODIFY `data_start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_conninfo` MODIFY `data_end_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_contactgroups` MODIFY `alias` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_contactnotificationmethods` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_contactnotificationmethods` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_contactnotificationmethods` MODIFY `command_args` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_contactnotifications` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_contactnotifications` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_contacts` MODIFY `alias` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_contacts` MODIFY `email_address` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_contacts` MODIFY `pager_address` varchar(64) NOT NULL default '';

ALTER TABLE `nagios_contactstatus` MODIFY `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_contactstatus` MODIFY `last_host_notification` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_contactstatus` MODIFY `last_service_notification` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_contact_addresses` MODIFY `address` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_contact_notificationcommands` MODIFY `command_args` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_customvariables` MODIFY `varname` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_customvariables` MODIFY `varvalue` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_customvariablestatus` MODIFY `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_customvariablestatus` MODIFY `varname` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_customvariablestatus` MODIFY `varvalue` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_dbversion` MODIFY `name` varchar(10) NOT NULL default '';
ALTER TABLE `nagios_dbversion` MODIFY `version` varchar(10) NOT NULL default '';

ALTER TABLE `nagios_downtimehistory` MODIFY `entry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_downtimehistory` MODIFY `author_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_downtimehistory` MODIFY `comment_data` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_downtimehistory` MODIFY `scheduled_start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_downtimehistory` MODIFY `scheduled_end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_downtimehistory` MODIFY `actual_start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_downtimehistory` MODIFY `actual_end_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_eventhandlers` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_eventhandlers` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_eventhandlers` MODIFY `command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_eventhandlers` MODIFY `command_line` TEXT NOT NULL default '';
ALTER TABLE `nagios_eventhandlers` MODIFY `output` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_externalcommands` MODIFY `entry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_externalcommands` MODIFY `command_name` varchar(128) NOT NULL default '';
ALTER TABLE `nagios_externalcommands` MODIFY `command_args` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_flappinghistory` MODIFY `event_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_flappinghistory` MODIFY `comment_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_hostchecks` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hostchecks` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hostchecks` MODIFY `command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hostchecks` MODIFY `command_line` TEXT NOT NULL default '';
ALTER TABLE `nagios_hostchecks` MODIFY `output` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hostchecks` MODIFY `perfdata` TEXT NOT NULL default '';

ALTER TABLE `nagios_hostgroups` MODIFY `alias` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_hosts` MODIFY `alias` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `display_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `address` varchar(128) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `check_command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `eventhandler_command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `failure_prediction_options` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `notes` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `notes_url` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `action_url` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `icon_image` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `icon_image_alt` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `vrml_image` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hosts` MODIFY `statusmap_image` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_hoststatus` MODIFY `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `output` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hoststatus` MODIFY `perfdata` TEXT NOT NULL default '';
ALTER TABLE `nagios_hoststatus` MODIFY `last_check` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `next_check` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_state_change` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_hard_state_change` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_time_up` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_time_down` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_time_unreachable` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `last_notification` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `next_notification` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_hoststatus` MODIFY `event_handler` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_hoststatus` MODIFY `check_command` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_instances` MODIFY `instance_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_instances` MODIFY `instance_description` varchar(128) NOT NULL default '';

ALTER TABLE `nagios_logentries` MODIFY `logentry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_logentries` MODIFY `entry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_logentries` MODIFY `logentry_data` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_logentries` MODIFY `logentry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_logentries` MODIFY `entry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_logentries` MODIFY `logentry_data` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_notifications` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_notifications` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_notifications` MODIFY `output` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_objects` MODIFY `name1` varchar(128) NOT NULL default '';
ALTER TABLE `nagios_objects` MODIFY `name2` varchar(128) NOT NULL default '';
ALTER TABLE `nagios_objects` DROP INDEX `objecttype_id`;
ALTER TABLE `nagios_objects` ADD UNIQUE `uniq_object` (`object_id`,`name1`,`name2`);

ALTER TABLE `nagios_processevents` MODIFY `event_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_processevents` MODIFY `program_name` varchar(16) NOT NULL default '';
ALTER TABLE `nagios_processevents` MODIFY `program_version` varchar(20) NOT NULL default '';
ALTER TABLE `nagios_processevents` MODIFY `program_date` varchar(10) NOT NULL default '';

ALTER TABLE `nagios_programstatus` MODIFY `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_programstatus` MODIFY `program_start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_programstatus` MODIFY `program_end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_programstatus` MODIFY `last_command_check` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_programstatus` MODIFY `last_log_rotation` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_programstatus` MODIFY `global_host_event_handler` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_programstatus` MODIFY `global_service_event_handler` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_runtimevariables` MODIFY `varname` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_runtimevariables` MODIFY `varvalue` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_scheduleddowntime` MODIFY `entry_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_scheduleddowntime` MODIFY `author_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_scheduleddowntime` MODIFY `comment_data` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_scheduleddowntime` MODIFY `scheduled_start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_scheduleddowntime` MODIFY `scheduled_end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_scheduleddowntime` MODIFY `actual_start_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_servicechecks` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicechecks` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicechecks` MODIFY `command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_servicechecks` MODIFY `command_line` TEXT NOT NULL default '';
ALTER TABLE `nagios_servicechecks` MODIFY `output` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_servicechecks` MODIFY `perfdata` TEXT NOT NULL default '';

ALTER TABLE `nagios_servicegroups` MODIFY `alias` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_services` MODIFY `display_name` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `check_command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `eventhandler_command_args` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `failure_prediction_options` varchar(64) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `notes` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `notes_url` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `action_url` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `icon_image` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_services` MODIFY `icon_image_alt` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_servicestatus` MODIFY `status_update_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `output` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_servicestatus` MODIFY `perfdata` TEXT NOT NULL default '';
ALTER TABLE `nagios_servicestatus` MODIFY `last_check` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `next_check` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_state_change` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_hard_state_change` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_time_ok` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_time_warning` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_time_unknown` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_time_critical` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `last_notification` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `next_notification` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_servicestatus` MODIFY `event_handler` varchar(255) NOT NULL default '';
ALTER TABLE `nagios_servicestatus` MODIFY `check_command` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_statehistory` MODIFY `state_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_statehistory` MODIFY `output` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_systemcommands` MODIFY `start_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_systemcommands` MODIFY `end_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_systemcommands` MODIFY `command_line` TEXT NOT NULL default '';
ALTER TABLE `nagios_systemcommands` MODIFY `output` varchar(255) NOT NULL default '';

ALTER TABLE `nagios_timedeventqueue` MODIFY `queued_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_timedeventqueue` MODIFY `scheduled_time` datetime NOT NULL default '1970-01-01 00:00:01';

ALTER TABLE `nagios_timedevents` MODIFY `queued_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_timedevents` MODIFY `event_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_timedevents` MODIFY `scheduled_time` datetime NOT NULL default '1970-01-01 00:00:01';
ALTER TABLE `nagios_timedevents` MODIFY `deletion_time` datetime NOT NULL default '1970-01-01 00:00:01';
