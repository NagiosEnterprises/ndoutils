SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_flappinghistory.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_flappinghistory.*
FROM `nagios_flappinghistory`
LEFT JOIN nagios_objects as obj1 ON nagios_flappinghistory.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_flappinghistory.instance_id=nagios_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY event_time DESC, event_time_usec DESC, flappinghistory_id DESC

