SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_hosts.host_object_id
,obj1.name1 AS host_name
FROM `nagios_hosts`
LEFT JOIN nagios_objects as obj1 ON nagios_hosts.host_object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_hosts.instance_id=nagios_instances.instance_id
WHERE nagios_hosts.config_type='1'
ORDER BY instance_name ASC, host_name ASC

