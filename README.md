# NDO Rewrite

There have been some attempts to make NDO faster along the way.

The most recent (of the time of this writing) attempt currently exists in the
`experimental-ndomod-db-handler` branch.

This was a somewhat successful attempt. We learned a lot, and what needs to
happen in order to actually speed this bus up.

Quite frankly, it's a complete rewrite. Development time (and debugging) will
be sped up possibly exponentially. Don't believe me? Try and compile that
aforementioned branch.


## What needs done

- [x] Base broker functionality (skeleton module)
- [x] MySQL connectivity and prepared statements
- [ ] Autoconf and Makefiles
- [ ] Configuration dumping
    - [ ] Original
    - [ ] Retained
- [ ] Status dumping
    - [ ] Program
    - [ ] Host
    - [ ] Service
    - [ ] Contact
- [ ] Historical dumping
    - [ ] Logging
    - [ ] External commands
    - [ ] Flapping
    - [ ] Downtime
    - [ ] Host check
    - [ ] Service check
    - [ ] Notification
    - [ ] Event handlers
    - [ ] Timed event
    - [ ] Process info
- [ ] Updated documentation
- [ ] Version bump script
- [ ] DB migration scripts
- [ ] CT/CI/CD for building
