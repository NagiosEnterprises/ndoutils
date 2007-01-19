SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_downtimehistory.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_downtimehistory.*
FROM `nagios_downtimehistory`
LEFT JOIN nagios_objects as obj1 ON nagios_downtimehistory.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_downtimehistory.instance_id=nagios_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY scheduled_start_time DESC, actual_start_time DESC, actual_start_time_usec DESC, downtimehistory_id DESC

