#!/bin/bash -e
# url="E:/DCube/mingw/mingw64/bin"
# url="E:/msys64/mingw64/bin"
# mingw_Make="${url}/mingw32-make.exe"

if [ ! -n "$1" ];then
	echo "--- make ---"
else
	echo "--- make clean ---"
	if [ $1="clear" ];then #输入参数 clear 时，清楚原来文件，重新编译
		rm -rf "$(pwd)/build"
	fi
fi


# export CXX="${url}/g++.exe"
# export CC="${url}/gcc.exe"

mkdir -p build
cd build

#"CMAKE_MAKE_PROGRAM"="${url}/mingw32-make.exe"
# or 
#"CMAKE_MAKE_PROGRAM"="${url}/make.exe"

# cmake -G "MSYS Makefiles" \
# 	-D "CMAKE_SYSTEM_NAME"="Windows"										\
# 	-D "CMAKE_C_COMPILER"="${url}/gcc.exe"									\
# 	-D "CMAKE_CXX_COMPILER"="${url}/g++.exe" 								\
# 	-D "CMAKE_MAKE_PROGRAM"=${mingw_Make}   								\
# 	../
	
# make -j$(nproc)
# ${mingw_Make} -j$(nproc)

cmake ../

echo "output path = $(pwd)/bin/win"