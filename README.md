# How to build

linux

```bash
$ git submodule update --init --recursive
$ cd buildscripts/linux-x86_64
$ ./build.sh
```

windows(build on ubuntu with mingw64)

```bash
$ sudo apt install libtool mingw-w64
$ git submodule update --init --recursive
$ cd buildscripts/linux-mingw64
$ ./build.sh
```

windows(build on windows with msys2/mingw64)

```bash
$ pacman -Sy --needed base-devel git mingw-w64-x86_64-gcc make libtool autoconf automake-wrapper cython
$ git submodule update --init --recursive
$ cd buildscripts/windows-mingw64
$ ./build.sh
```

> NOTE: `libusbmuxd` must be compiled as shared library

macOS(x86_64)

```bash
$ brew install libtool
$ brew install automake
$ cd buildscripts/darwin-x86_64
$ ./build.sh
```

macOS(arm64)

```bash
$ brew install libtool
$ brew install automake
$ cd buildscripts/darwin-arm64
$ ./build.sh
```


**特别注意：**

echo "如果编译失败, 请修改 /usr/share/aclocal/libtool.m4: "

echo "在合适的位置加上"

echo "  lt_cv_deplibs_check_method=pass_all "

echo "ref: https://lists.gnu.org/archive/html/libtool/2012-07/msg00000.html"





```
mingw* | pw32*)
  # Base MSYS/MinGW do not provide the 'file' command needed by
  # func_win32_libid shell function, so use a weaker test based on 'objdump',
  # unless we find 'file', for example because we are cross-compiling.
  if ( file / ) >/dev/null 2>&1; then
    lt_cv_deplibs_check_method='file_magic ^x86 archive import|^x86 DLL'
    lt_cv_file_magic_cmd='func_win32_libid'
  else
    # Keep this pattern in sync with the one in func_win32_libid.
    lt_cv_deplibs_check_method='file_magic file format (pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)'
    lt_cv_file_magic_cmd='$OBJDUMP -f'
  fi
  lt_cv_deplibs_check_method=pass_all     # 在这里加这一行
  ;;

```

