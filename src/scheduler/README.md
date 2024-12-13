# The FTS scheduler

The FTS scheduler daemon is implemented as a Python package.  Type the following
in order to run the daemon:
```
python fts_scheduler
```

Type the following to carry out a minimum level of checks with pylint:
```
PYTHONPATH=fts_scheduler pylint --output-format colorized --disable C,R,W fts_scheduler
```
