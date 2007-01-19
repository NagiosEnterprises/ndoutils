SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_statehistory.object_id
,obj1.objecttype_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_statehistory.*
FROM `nagios_statehistory`
LEFT JOIN nagios_objects as obj1 ON nagios_statehistory.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_statehistory.instance_id=nagios_instances.instance_id
ORDER BY state_time DESC, state_time_usec DESC

