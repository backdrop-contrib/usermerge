User Merge
======================

This module helps users to merge together duplicate accounts. It uses a hook
system so other modules can do whatever they need to manage their relationships
to users.

Users with the right permissions can choose how each user
property should be merged. This includes the ability to merge fields,
referencing entities, and other entities owned by the selected users.

Support for several existing Backdrop modules is built in (see below). An API is
provided so that other modules can provide support for merging their data
associated with users when two users are merged. See `usermerge.api` for more
information.

Installation
------------

- Install this module using [the official Backdrop CMS instructions](https://backdropcms.org/guide/modules).

- Visit the configuration page under Administration > Configuration > People >
User Merge (admin/config/people/usermerge) and select which core properties of
the user entity should be exposed in the review table. By default, no properties
are exposed.

Integration with Other Modules
------------------------------

Integration with other modules is contained in module-specific files in the
`includes` directory.

Modules supported out of the box:

- Entity Reference (`entity_reference`)
- Profile (`profile`), which is the replacement for the Drupal Profile2 module (`profile2`)
- Real Name (`realname`)
- User Reference (`user_references`), which is a submodule of References (`references`)

Additional modules can be supported by providing their own
`<module>.usermerge.inc` files. This module also provides the `usermerge_do()`
function, which, given two user objects, merges them preserving information from
the "new" account.

Core-specific functionality (default user properties, fields) is managed in
`usermerge.usermerge.inc`, which also includes support for entities that have a
`uid` column, and basic display support for non-default user properties that
aren't structured like fields (such as `metatags`).

See some of the existing Backdrop `<module>_usermerge` modules for examples of
how to integrate with this module.

Documentation
-------------

Additional documentation is located in [the Wiki](https://github.com/backdrop-contrib/usermerge/wiki/Documentation).

Differences from Drupal 7
-------------------------

These modules were supported by the Drupal 7 version but have been dropped
because there are no corresponding Backdrop modules:

- Multiple E-mail (`multiple_email`)
- Profile (`profile`); the Backdrop Profile module replaces the Drupal Profile2 module
- RDF (`rdf`)
- User Points (`userpoints`)

Issues
------

Bugs and feature requests should be reported in [the Issue Queue](https://github.com/backdrop-contrib/usermerge/issues).

Current Maintainers
-------------------

- [Robert J. Lang](https://github.com/bugfolder).

Credits
-------

- Ported to Backdrop CMS by [Robert J. Lang](https://github.com/bugfolder).
- Originally written for Drupal by [Greg Knaddison (greggles)](https://www.drupal.org/u/greggles).

License
-------

This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.

