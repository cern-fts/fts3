version=${shell grep '^Version:' ./specs/fts.spec | awk '{print $$2}' }
name=fts
rpmbuild=${shell pwd}/rpmbuild
MOCKENV=epel-6-cernonly-x86_64
MOCKARGS=--trace --verbose

dist: clean
	tar --create --gzip --directory ../.. --file ./${name}-${version}.tar.gz --exclude=./packaging --exclude .svn .

clean:
	@rm -rf $(rpmbuild)
	@rm -f ${name}-${version}.tar.gz

prepare: dist
	@mkdir -p  $(rpmbuild)/RPMS/noarch
	@mkdir -p  $(rpmbuild)/SRPMS/
	@mkdir -p  $(rpmbuild)/SPECS/
	@mkdir -p  $(rpmbuild)/SOURCES/
	@mkdir -p  $(rpmbuild)/BUILD/
	mv ${name}-${version}.tar.gz $(rpmbuild)/SOURCES 

srpm: prepare ./specs/$(name).spec
	rpmbuild --nodeps -bs --define='_topdir ${rpmbuild}' ./specs/$(name).spec

rpm: srpm
	rpmbuild --nodeps -bb --define='_topdir ${rpmbuild}' ./specs/$(name).spec

mock: srpm
	/usr/bin/mock $(MOCKARGS) -r $(MOCKENV) --rebuild $(rpmbuild)/SRPMS/$(name)-$(version)-*.src.rpm


