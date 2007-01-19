SELECT 
nagios_instances.instance_id
,nagios_instances.instance_name
,nagios_hostgroups.hostgroup_id
,nagios_hostgroups.hostgroup_object_id
,obj1.name1 AS hostgroup_name
,nagios_hostgroups.alias AS hostgroup_alias
,nagios_hosts.host_object_id 
,obj2.name1 AS host_name
FROM `nagios_hostgroups` 
INNER JOIN nagios_hostgroup_members ON nagios_hostgroups.hostgroup_id=nagios_hostgroup_members.hostgroup_id 
INNER JOIN nagios_hosts ON nagios_hostgroup_members.host_object_id=nagios_hosts.host_object_id
INNER JOIN nagios_objects as obj1 ON nagios_hostgroups.hostgroup_object_id=obj1.object_id
INNER JOIN nagios_objects as obj2 ON nagios_hostgroup_members.host_object_id=obj2.object_id
INNER JOIN nagios_instances ON nagios_hostgroups.instance_id=nagios_instances.instance_id