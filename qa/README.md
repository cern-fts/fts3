FTS3 QA
=======

In order to get coverage information from the functional tests, we need
a version of FTS3 compiled with the `--coverage` flag, so the binaries
are properly instrumented.

To facilitate the extraction of the coverage information, the Dockerfiles
under this directory allow to start an instrumented fts3 server, together
with a supporting MySQL instances.

## Build the images

### MySQL image
```bash
docker build --rm=true --pull -f Dockerfile_mysql -t fts3-qa-mysql ..
```

### QA image
```bash
docker build --rm=true --pull -f Dockerfile -t fts3-qa ..
```

## Run the instrumented FTS3
```bash
FTS3_HOST=`hostname -f` STOMP_PASSWORD=xxx FTS3_USER=`id -un` FTS3_GROUP=`id -gn` docker-compose up
```

Note that the defaults should be good enough, although, of course, insecure!

Once the mini-instance is up, you can run the
[tests](https://gitlab.cern.ch/fts/fts-tests), and recover the coverage
information from /tmp/fts3qa/coverage.

It may be better if you run docker-compose up first for the db, and then for FTS.
Otherwise, docker will likely SIGKILL supervisord without a chance to an ordered
shutdown.

Once the tests are done, shutdown the instance, first the server, and then MySQL.

```bash
docker-compose stop -t 180 fts3-qa
docker-compose stop -t 180 fts3-qa-mysql
```

And the coverage information will be under `/tmp/fts3qa/coverage/*.xml` by
default.
It can be recovered, and used to send the QA data to SonarQube.
