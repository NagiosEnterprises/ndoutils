SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_scheduleddowntime.object_id
,obj1.objecttype_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_scheduleddowntime.*
FROM `nagios_scheduleddowntime`
LEFT JOIN nagios_objects as obj1 ON nagios_scheduleddowntime.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_scheduleddowntime.instance_id=nagios_instances.instance_id
ORDER BY scheduled_start_time DESC, scheduleddowntime_id DESC

