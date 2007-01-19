SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_notifications.object_id
,obj1.objecttype_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,obj2.name1 AS contact_name
,nagios_contactnotifications.*
FROM `nagios_contactnotifications`
JOIN nagios_notifications ON nagios_contactnotifications.notification_id=nagios_notifications.notification_id
LEFT JOIN nagios_objects as obj1 ON nagios_notifications.object_id=obj1.object_id
LEFT JOIN nagios_objects as obj2 ON nagios_contactnotifications.contact_object_id=obj2.object_id
LEFT JOIN nagios_instances ON nagios_notifications.instance_id=nagios_instances.instance_id
ORDER BY start_time DESC, start_time_usec DESC

