SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_services.host_object_id
,obj1.name1 AS host_name
,nagios_services.service_object_id
,obj1.name2 AS service_description
,nagios_servicestatus.*
FROM `nagios_servicestatus`
LEFT JOIN nagios_objects as obj1 ON nagios_servicestatus.service_object_id=obj1.object_id
LEFT JOIN nagios_services ON nagios_servicestatus.service_object_id=nagios_services.service_object_id
LEFT JOIN nagios_instances ON nagios_services.instance_id=nagios_instances.instance_id
WHERE nagios_services.config_type='1'
ORDER BY instance_name ASC, host_name ASC, service_description ASC