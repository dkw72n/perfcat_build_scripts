#!/bin/bash

MY_DIR=$(dirname `realpath $0`)
# echo "MY_DIR=${MY_DIR}"
TARGET_DIR=${MY_DIR}/../../built/linux-mingw64/

mkdir -p ${TARGET_DIR} || true
export PKG_CONFIG_PATH=${TARGET_DIR}/lib/pkgconfig

build_libplist() {
  (
  set -ex
  cd ${MY_DIR}/../../libplist
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --host=x86_64-w64-mingw32 --without-cython
  make -j2
  make install
  )
}

build_libusbmuxd(){
  (
  set -ex
  cd ${MY_DIR}/../../libusbmuxd
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --host=x86_64-w64-mingw32
  make -j2
  make install
  )
}

build_openssl(){
  (
  set -ex
  cd ${MY_DIR}/../../openssl
  make clean || true
  ./Configure mingw64 --prefix=${TARGET_DIR} --cross-compile-prefix=x86_64-w64-mingw32- --openssldir=${TARGET_DIR}/openssl
  make -j2
  make install
  )
}

build_libimobiledevice(){
  (
  set -ex
  cd ${MY_DIR}/../../libimobiledevice
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --host=x86_64-w64-mingw32 --without-cython
  make -j2
  make install
  )
}

case $1 in 
  libplist)
    build_libplist
    ;;
  libusbmuxd)
    build_libusbmuxd
    ;;
  openssl)
    build_openssl
    ;;
  libimobiledevice)
    build_libimobiledevice
    ;;
  all|"")
    (
    set -ex
    build_libplist
    build_libusbmuxd
    build_openssl
    build_libimobiledevice
    )
    ;;
  *)
    echo "$0 [libplist|libusbmuxd|openssl|libimobiledevice|all]"
esac
