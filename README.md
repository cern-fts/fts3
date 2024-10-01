FTS3
====
FTS3 is the service responsible for globally distributing the majority of the
LHC data across the WLCG infrastructure. Is a low level data movement service,
responsible for reliable bulk transfer of files from one site to another while
allowing participating sites to control the network resource usage.

## Vagrant
To ease the development of FTS3, we provide here a Vagrantfile. Using it should
be straight forward, provided you have installed Vagrant and a provider (i.e. VirtualBox).

Once you have them installed, just run

```bash
vagrant up
```

And ready to go. However, if you want to be able to compile the changes you make in your
local copy (with your preferred IDE/editor), you may need to run

```bash
vagrant rsync-auto
```

after the instance is up. This will synchronize the files.

*Note:* Now the image is based on bento/centos, which by default installs
Virtual Box Guest Additions, making unnecessary this command.

To enter the vagrant instace, just run on a separate shell 

```bash
vagrant ssh
```

and you will be there. The source code will be under `/vagrant`. You should do the builds
somewhere else, though, since that folder is going to be synchronized, and you will lose
the artifacts and build configuration.

You can do the build in `/home/vagrant/build`, for instance.

```bash
cd /home/vagrant
mkdir build
cd  build
cmake /vagrant/ -DALLBUILD=ON`
make
```

If you want to get rid of the image, run `vagrant destroy`

### Continuous Integration

The project uses Gitlab-CI for CI/CD. The [pipeline][1] runs for every push, in every branch:
- static\_code\_analysis:
  - black: checks code must be formatted with `black`
  - pylint: checks for syntax errors (runs for every supported Python3 version)
    - If you are sure that pylint is mistaken, add `# pylint: skip-file` at the beginning of the relevant file
- build: builds server RPMs
- docker: builds Docker images
- test: runs unit-tests
- publish: uploads server RPMs to the FTS testing repository

Merge requests will proceed only if the pipeline succeeds.
In case of emergency the pipeline can be [skipped][2].

The `black` and `pylint` jobs run in a container from the image tagged as `ci`.
The `Dockerfile` is located at `.gitlab-ci/docker/Dockerfile` and the image is
hosted in the container registry of this project. The image contains the
pre-installed Python environment in order to speed up CI execution. When a new
environment is desired (such as new or upgraded dependencies), a new Docker
image must created by locally executing the
`.gitlab-ci/docker/create-ci-image.sh` script, for example:
```
dnf install podman-docker

cd .gitlab-ci/docker
./create-ci-image.sh
```
Use the `ftssuite` account when asked by the
`docker login gitlab-registry.cern.ch/fts/fts3` command for your username and
password.

## References
* Web page: http://fts3-service.web.cern.ch/
* Documentation: http://fts3-docs.web.cern.ch/fts3-docs/docs/cli.html
* Developers guide: http://fts3-docs.web.cern.ch/fts3-docs/docs/developers.html
* Ticket handling in [JIRA](https://its.cern.ch/jira/browse/FTS/?selectedTab=com.atlassian.jira.jira-projects-plugin:summary-panel)
* Continuous integration in [Jenkins](https://jenkins-fts-dmc.web.cern.ch/)
* Monitored by the [Dashboard](http://dashb-fts-transfers.cern.ch/ui/)
* For help, contact fts-support@cern.ch, or fts-devel@cern.ch

[1]: https://gitlab.cern.ch/fts/fts3/-/pipelines
[2]: https://docs.gitlab.com/ee/ci/yaml/#skipping-jobs
