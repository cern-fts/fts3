# This script assumes that:
# - pwd is the top source directory


mkdir -p  ./src/cli/external
cd ./src/cli/external
wget https://curl.haxx.se/download/curl-7.65.2.tar.gz
tar zxvf curl-7.65.2.tar.gz
mv curl-7.65.2 curl
cd curl
cmake3 . -DCMAKE_INSTALL_PREFIX=/tmp/curl/bogusinstall -DBUILD_CURL_EXE=false -DBUILD_TESTING=false -DBUILD_SHARED_LIBS=false  -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC -DCURL_DISABLE_LDAP=ON
make -j2 install
