SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_notifications.object_id AS service_object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_notifications.*
FROM `nagios_notifications`
LEFT JOIN nagios_objects as obj1 ON nagios_notifications.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_notifications.instance_id=nagios_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY start_time DESC, start_time_usec DESC