SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_eventhandlers.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_eventhandlers.*
FROM `nagios_eventhandlers`
LEFT JOIN nagios_objects as obj1 ON nagios_eventhandlers.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_eventhandlers.instance_id=nagios_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY start_time DESC, start_time_usec DESC, eventhandler_id DESC

