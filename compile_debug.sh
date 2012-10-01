#rm -fr build
#mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr  -DLIB_SUFFIX=64 -DCMAKE_SKIP_RPATH:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=ON  $1
make 2>&1 | sed -e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' -e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%' 
cd ../


