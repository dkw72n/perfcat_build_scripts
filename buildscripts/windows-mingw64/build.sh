#!/bin/bash

MY_DIR=$(dirname `realpath $0`)
# echo "MY_DIR=${MY_DIR}"
TARGET_DIR=${MY_DIR}/../../built/windows-mingw64/

mkdir -p ${TARGET_DIR} || true
export PKG_CONFIG_PATH=${TARGET_DIR}/lib/pkgconfig

build_libplist() {
  (
  set -ex
  cd ${MY_DIR}/../../libplist
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --without-cython
  make -j4
  make install
  )
}

build_libimobiledevice_glue() {
  (
  set -ex
  cd ${MY_DIR}/../../libimobiledevice-glue
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR}
  make -j4
  make install
  )
}

build_libusbmuxd(){
  (
  set -ex
  cd ${MY_DIR}/../../libusbmuxd
  git checkout -- .
  #git apply ../patch/libusbmuxd-mingw-caseawearness.patch
  git apply ../patch/libusbmuxd-windows-errno-bug.patch
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} CFLAGS="-fPIC -D_WIN32_WINNT=0x601" # --enable-shared=no 0x0601=Windows 7
  make -j4 V=1
  make install
  rm tools/*.exe -f
  git checkout -- .
  )
}

build_openssl(){
  (
  set -ex
  cd ${MY_DIR}/../../openssl
  make clean || true
  ./Configure no-shared mingw64 --prefix=${TARGET_DIR} --openssldir=${TARGET_DIR}/openssl
  make -j6
  make install_sw
  )
}

build_libimobiledevice(){
  (
  set -ex
  cd ${MY_DIR}/../../libimobiledevice
  git checkout -- .
  make clean || true
  sed -i "s/idevice\.c idevice\.h/idevice.c ext.c idevice.h/" src/Makefile.am
  cp ../patch/ext.c src/
  cp ../patch/ext.h include/libimobiledevice
  # git apply ../patch/libimobiledevice-socket-mingw-compatibility.patch
  # git apply ../patch/libimobiledevice-openssl-concurrently-read-write-bug-workaround.patch
  ./autogen.sh --prefix=${TARGET_DIR} --without-cython --enable-debug LIBS="-lcrypt32" LDFLAGS="-Wl,--export-all-symbols" CFLAGS="-D_WIN32_WINNT=0x601"
  make -j4 V=1
  make install
  git checkout -- .
  rm tools/*.exe -f
  rm src/ext.c include/libimobiledevice/ext.h
  )
}

case $1 in 
  libplist)
    build_libplist
    ;;
  libimobiledevice_glue)
    build_libimobiledevice_glue
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
    build_libimobiledevice_glue
    build_libusbmuxd
    build_openssl
    build_libimobiledevice
    )
    ;;
  *)
    echo "$0 [libplist|libimobiledevice_glue|libusbmuxd|openssl|libimobiledevice|all]"
esac
