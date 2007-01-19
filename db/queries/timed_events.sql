SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_timedevents.event_type
,nagios_timedevents.scheduled_time
,nagios_timedevents.event_time
,nagios_timedevents.event_time_usec
,nagios_timedevents.recurring_event
,obj1.objecttype_id
,nagios_timedevents.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
FROM `nagios_timedevents`
LEFT JOIN nagios_objects as obj1 ON nagios_timedevents.object_id=obj1.object_id
LEFT JOIN nagios_instances ON nagios_timedevents.instance_id=nagios_instances.instance_id
WHERE scheduled_time < NOW()
ORDER BY scheduled_time DESC, timedevent_id DESC

