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
  make -j4
  make install
  )
}

build_libusbmuxd(){
  (
  set -ex
  cd ${MY_DIR}/../../libusbmuxd
  git checkout -- .
  git apply ../patch/libusbmuxd-socket-mingw-compatibility.patch
  make clean || true
  ./autogen.sh --prefix=${TARGET_DIR} --host=x86_64-w64-mingw32 CFLAGS="-fPIC -D_WIN32_WINNT=0x601" --enable-shared=no
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
  ./Configure no-shared mingw64 --prefix=${TARGET_DIR} --cross-compile-prefix=x86_64-w64-mingw32- --openssldir=${TARGET_DIR}/openssl
  make -j6
  make install
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
  git apply ../patch/libimobiledevice-socket-mingw-compatibility.patch
  ./autogen.sh --prefix=${TARGET_DIR} --host=x86_64-w64-mingw32 --without-cython --enable-debug-code LIBS="-lcrypt32" LDFLAGS="-Wl,--export-all-symbols" CFLAGS="-D_WIN32_WINNT=0x601"
  make -j4 V=1
  make install
  git checkout -- .
  rm src/ext.c include/libimobiledevice/ext.h
  )
}

echo "如果编译失败, 请修改 /usr/share/aclocal/libtool.m4: "
echo "在合适的位置加上"
echo "   lt_cv_deplibs_check_method=pass_all  "
echo "ref: https://lists.gnu.org/archive/html/libtool/2012-07/msg00000.html"

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
