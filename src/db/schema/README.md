Database schema
===============

## Type of sql scripts
Database schemas are shipped as a set of two types of files: full
schema and diffs.

Full schemas can be run on an empty database and populate it with the
tables, indexes, etc.

Diff schemas must be run over an existing database, and modify it.

Normally, a database can be created from scratch using the
highest schema version (i.e. fts-schema-1.0.0.sql), and applying *all*
diff with a higher version (fts-diff-1.1.0.sql) incrementally.

For instance

```
fts-schema-1.0.0.sql
fts-diff-1.0.1.sql
fts-diff-1.1.0.sql
```

For convenience, the tool `fts-database-upgrade.py` can be used.

## Versioning
We use a three number version: major, minor and patch.
When to increment each?

  * **major** version must be increased when older versions of fts will *not*
  work with the new schema, and probably the new version of fts will
  not work with the older either. These sort of upgrades require downtime.
  
  * **minor** version must be increased when the production version can still run with the new schema,
  but the newer fts version will *not* be able to run with the old schema.
  The upgrade can be done in two steps to avoid downtime: first database, then fts.
  
  * **patch** version must be increased for minor changes. Both older and
  newer fts version can work with the schema patched or not. For instance,
  indexes changes, dropping unused columns...
