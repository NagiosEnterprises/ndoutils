# NDO Development Notes

Because of the nature of Nagios and NDO internal structures, putting detailed comments for one function generally applies to other functions as well.

As an example, there are handler functions for contacts, hosts, services, etc. The logic for each function is very similar, and most have the same quirks shared between them. Where should the notes go in this case? We think it makes more sense to have detailed explanations here and reference this document from the comments.


## Startup / Object Writers

On startup, Nagios sends some broker data indicating that startup just occured. Once this happens, all of the object definitions are truncated and then looped over and re-inserted. The only thing that remains is the objects in `nagios_objects` - these are set to `is_active = 0` and then once the re-inserting happens, the definitions are activated. This is primarily to keep the proper object_ids over the lifetime of the objects.

You'll notice some inconsistencies in the way the functions work for writing objects. Namely, contacts, hosts, and services. The reason for this is simple - on small systems (< \~2500 objects), the queries don't take much time at all (< 1 second in many cases) - however on very large systems this process has been recorded taking over a minute on moderate hardware. The functions responsible for inserting the contacts, hosts, and services took the longest time, and so a better solution was necessary.

These functions make use of bulk insertion statements - as we loop over the objects, we build a large query and once it's reached a limit (defined in `ndo.cfg`) it sends all that data.


### `compute-startup-hash.sh`

Many times, we restart our Nagios systems when no object definitions have changed. We've introduced this simple script to allow NDO to determine if it needs to truncate and then re-insert the definitions on startup.

The script is called during NEB initialization, and then sets a global flag to determine if we should ignore some of the process startup broker data (namely the ones that tell us to start writing the data).

This option is not enabled by default. It can be turned on by adding `enable_startup_hash = 1` in `ndo.cfg`.


### `UPDATE_QUERY_X_POS`