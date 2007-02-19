ALTER TABLE `nagios_host_contacts` DROP INDEX `instance_id`;

ALTER TABLE `nagios_host_contacts` ADD UNIQUE (
`instance_id` ,
`host_id` ,
`contact_object_id`
);

ALTER TABLE `nagios_service_contacts` DROP INDEX `instance_id`; 

ALTER TABLE `nagios_service_contacts` ADD UNIQUE (
`instance_id` ,
`service_id` ,
`contact_object_id`
);

ALTER TABLE `nagios_hostescalation_contacts` DROP INDEX `instance_id` ;

ALTER TABLE `nagios_hostescalation_contacts` ADD UNIQUE (
`instance_id` ,
`hostescalation_id` ,
`contact_object_id`
);

ALTER TABLE `nagios_serviceescalation_contacts` DROP INDEX `instance_id` ;

ALTER TABLE `nagios_serviceescalation_contacts` ADD UNIQUE (
`instance_id` ,
`serviceescalation_id` ,
`contact_object_id`
);

ALTER TABLE `nagios_services` ADD `host_object_id` INT NOT NULL AFTER `config_type` ;

-- Start of mods from 1.4b3

ALTER TABLE `nagios_hosts` ADD `alias` VARCHAR( 64 ) NOT NULL AFTER `host_object_id` ;
