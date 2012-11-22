rm -fr build
mkdir build
cd build
cmake -DALLBUILD=ON -D CMAKE_BUILD_TYPE=Release ../
make 2>&1 | sed -e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' -e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%'
cd ../
