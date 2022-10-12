# How to build

## linux

```bash
$ git submodule update --init --recursive
$ cd buildscripts/linux-x86_64
$ ./build.sh
```

## windows(build on ubuntu with mingw64)

```bash
$ sudo apt install libtool mingw-w64
$ git submodule update --init --recursive
$ cd buildscripts/linux-mingw64
$ ./build.sh
```

## windows(build on windows with [msys2/mingw64](https://www.msys2.org/))

1. 安装依赖, 打开 `msys2.exe` 执行如下命令:
```bash
$ pacman -Sy --needed base-devel git mingw-w64-x86_64-gcc make libtool autoconf automake-wrapper cython
```

2. 开始编译, 打开 `mingw64.exe` 执行如下命令，:
```bash
$ git clone git@github.com:dkw72n/perfcat_build_scripts.git
$ git submodule update --init --recursive
$ cd buildscripts/windows-mingw64
$ ./build.sh
```
> NOTE: 如下是在windows下拉代码则必须关闭git的autocrlf, 否则换行符会引起编译错误.

> NOTE: `libusbmuxd` must be compiled as shared library

## macOS(x86_64)

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

