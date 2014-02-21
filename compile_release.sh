rm -fr build
mkdir build
cd build
cmake28 -DBoost_NO_BOOST_CMAKE=ON -DALLBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ../
make 2>&1 | sed -e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' -e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%'
cd ../
