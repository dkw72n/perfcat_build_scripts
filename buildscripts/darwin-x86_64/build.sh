#!/bin/bash

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

MY_DIR=$(dirname `realpath $0`)
#echo "MY_DIR=${MY_DIR}"
TARGET_DIR=${MY_DIR}/../../built/darwin-x86_64/

mkdir -p ${TARGET_DIR} || true
export PKG_CONFIG_PATH=${TARGET_DIR}/lib/pkgconfig

build_libplist() {
  (
  set -ex
  cd ${MY_DIR}/../../libplist
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --without-cython
  make -j2
  make install
  )
}

build_libusbmuxd(){
  (
  set -ex
  cd ${MY_DIR}/../../libusbmuxd
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR}
  make -j2
  make install
  )
}

build_openssl(){
  (
  set -ex
  cd ${MY_DIR}/../../openssl
  make clean || true
  ./Configure darwin64-x86_64-cc --prefix=${TARGET_DIR} --openssldir=${TARGET_DIR}/openssl
  make -j2
  make install
  )
}

build_libimobiledevice(){
  (
  set -ex
  cd ${MY_DIR}/../../libimobiledevice
  git checkout -- .
  make clean || true
  sed -i "" "s/idevice\.c idevice\.h/idevice.c ext.c idevice.h/" src/Makefile.am
  cp ../patch/ext.c src/
  cp ../patch/ext.h include/libimobiledevice
  # git apply ../patch/libimobiledevice-socket-mingw-compatibility.patch
  ./autogen.sh --prefix=${TARGET_DIR} --without-cython
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
