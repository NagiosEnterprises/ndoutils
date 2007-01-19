SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_commenthistory.object_id
,obj1.objecttype_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,nagios_commenthistory.*
FROM `nagios_commenthistory`
LEFT JOIN nagios_objects as obj1 ON nagios_commenthistory.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_commenthistory.instance_id=nagios_instances.instance_id
ORDER BY entry_time DESC, entry_time_usec DESC, commenthistory_id DESC

