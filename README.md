# NDO 3

[![Build Status](https://travis-ci.org/NagiosEnterprises/ndoutils.svg?branch=ndo-3)](https://travis-ci.org/NagiosEnterprises/ndoutils)

We've rewritten NDO! One of the major changes is that we no longer use `ndo2db`.

The architecture of NDO used to be something like this:

1. An event happens (check result, etc.) in Nagios
1. The event broker picks up the data and passes it to `ndomod.o` (broker module)
1. `ndomod.o` checks to make sure it is processing that kind of data
1. Then it compiles the broker data into a string
1. It sends that string over a (tcp or unix) socket that `ndo2db` is listening on
1. The fork of `ndo2db` that's listening pushes the string into the kernel message queue
1. The fork of `ndo2db` that waits for kernel messages pops the message and compiles it into an ndo database object
1. Once all of the data has been successfully read, it performs a database insert
1. Rinse and repeat

The architecture of NDO is now something like this:

1. An event happens in Nagios
1. The event broker picks up the data and passes it to `ndo.o`
    * **Note:** No checking is performed since each callback is registered to a unique handler function
1. The handler function performs the database insert with the broker data directly

A few other important changes to note:

* Table prefix options have gone away (all table names are hardcoded)
* Database queries have changed to prepared statements
* The code was originally written to support database abstraction - but the only database ever utilized was MySQL/MariaDB. We've opted to remove the abstraction and use direct MySQL function calls (we may add support for additional databases in the future)
* For clarity, we've removed complicated dependencies for additional operating systems
    * Based on community involvement or public outcry, we'll add these dependencies back in
* Support for Nagios versions 2.x - 3.x has been removed
    * In fact, any version of Nagios less than 4.4.0 is untested and unlikely to work properly

This rewrite is still a work in progress (with quite a bit left to do), but we encourage you to reach out and help/contribute however you can. Excited would-be alpha testers can send an email to [devteam@nagios.com](mailto:devteam@nagios.com) indicating their willingness to participate!

As we get closer to alpha, more documentation will be provided regarding contributions and user testing. In the meantime - if there is something on the list below that you can help with, please feel free to open a pull request.

## What's left to do

- [ ] Unit tests (100% coverage)
- [ ] CI/CD pipeline
- [ ] Technical documentation
- [ ] Ubuntu / Debian / CentOS / RedHat support
- [ ] Upgrade scripts (database migration) rewritten
- [ ] Database schema scrutinized and changes finalized (more or less indexes primarily)

## Contributing

If you wish to help out with this branch before it makes it to pre-alpha:

* **First, you must Have a sane build environment**

Then, getting everything built and installed should be rather easy:

1. `git clone https://github.com/NagiosEnterprises/ndoutils.git`
1. `git checkout ndo-3` (checkout the right branch)
1. `autoconf` (generate configure and makefiles)
1. `./configure --enable-testing` (if you're helping, you should enable debugging)
1. `make inittest` (this initializes the database with user `ndo`, pass `ndo`, and database `ndo`)
1. `make ndo.so` (compile the neb module)
1. `cp src/ndo.so /usr/local/nagios/bin/` (copy compiled neb module to nagios space)
1. `cp config/ndo.cfg-sample /usr/local/nagios/etc/ndo.cfg` (copy over new config)
1. `echo "broker_module=/usr/local/nagios/bin/ndo.so /usr/local/nagios/etc/ndo.cfg"` (enable the broker module and point to new config)
1. `sed -i 's/^broker_module.*ndomod.cfg.*/#&/' /usr/local/nagios/etc/nagios.cfg` (this gets rid of any previous ndo configs)

Then, from there - you can either create Issues or Pull Requests - or if you have a question you can reach us at devteam@nagios.com.

You can also read some of the [development notes in src/README.md](https://github.com/NagiosEnterprises/ndoutils/blob/ndo-3/src/README.md).

This is mainly here for posterity and our own development teams sake, but when adding functions/conditionals/etc. that reference the broker data (e.g.: log data, host status data, etc) - the preference is to keep each list in the same order. That order is officially based off the order of the broker functions from `nagioscore/base/broker.c`:

1. `program_state`
1. `timed_event`
1. `log_data`
1. `system_command`
1. `event_handler`
1. `host_check`
1. `service_check`
1. `comment_data`
1. `downtime_data`
1. `flapping_data`
1. `program_status`
1. `host_status`
1. `service_status`
1. `contact_status`
1. `notification_data`
1. `contact_notification_data`
1. `contact_notification_method_data`
1. `adaptive_program_data` *unused*
1. `adaptive_host_data` *unused*
1. `adaptive_service_data` *unused*
1. `adaptive_contact_data` *unused*
1. `external_command`
1. `aggregated_status_data` *unused*
1. `retention_data` *unused*
1. `acknowledgement_data`
1. `statechange_data`

*Pro-tip:* You can generate this list with the following command:

```
cat base/broker.c | grep "^void broker_\|^int broker_" | sed 's/^\w* //' | sed 's/^broker_//' | sed 's/(.*//'
```

Or even better:

```
cat base/broker.c | grep "^void broker_\|^int broker_" | sed 's/^\w* //' | sed 's/^broker_//' | sed 's/(.*//' | grep -v "^adaptive" | grep -v "^aggregated" | grep -v "^retention"
```
