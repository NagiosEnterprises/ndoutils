SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_timedeventqueue.event_type
,nagios_timedeventqueue.scheduled_time
,nagios_timedeventqueue.recurring_event
,obj1.objecttype_id
,nagios_timedeventqueue.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
FROM `nagios_timedeventqueue`
LEFT JOIN nagios_objects as obj1 ON nagios_timedeventqueue.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_timedeventqueue.instance_id=nagios_instances.instance_id
ORDER BY scheduled_time ASC, timedeventqueue_id ASC

