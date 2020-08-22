#!/bin/bash

BUILD_COMMON="-o test_bsearch main.cpp rand.cpp"

if [[ ${MACHTYPE} =~ "-apple-darwin" ]]; then :
	# Darwin has its timer framework linked in by default
else
	BUILD_COMMON+=" -lrt"
fi

# avoid thumb on arm
UNAME_MACHINE=`uname -m`

if [[ $UNAME_MACHINE == "armv7l" ]]; then
	CXXFLAGS+=(
		-marm
		-mfpu=neon
	)
elif [[ $UNAME_MACHINE == "aarch64" ]] && [[ ${HOSTTYPE:0:3} == "arm" ]]; then
	# for armv8 devices with aarch64 kernels + armv7 userspaces
	CXXFLAGS+=(
		-marm
		-mfpu=neon
	)
fi

if [[ $1 == "debug" ]]; then
	CXXFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG)
else
	# enable some optimisations that may or may not be enabled by the global optimisation level of choice in this compiler version
	CXXFLAGS+=(
		-fno-exceptions
		-fno-rtti
		-ffast-math
		-fstrict-aliasing
		-fstrict-overflow
		-funroll-loops
		-fomit-frame-pointer
		-O3
		-DNDEBUG)
fi

source cxx_util.sh

if [[ $HOSTTYPE == "aarch64" ]] || [[ ${HOSTTYPE:0:3} == "arm" ]]; then
	cxx_uarch_arm
else
	CXXFLAGS+=(
		-march=native
		-mtune=native
		-DCACHELINE_SIZE=64
	)
fi

if [[ $1 == "gcc" ]]; then
	BUILD_CMD="g++ "$BUILD_COMMON" "${CXXFLAGS[@]}" -fpermissive"
elif [[ $1 == "clang" ]]; then
	BUILD_CMD="clang++ "$BUILD_COMMON" "${CXXFLAGS[@]}" -Wno-shift-op-parentheses"
elif [[ $1 == "debug" ]]; then
	BUILD_CMD="clang++ "$BUILD_COMMON" "${CXXFLAGS[@]}
else
	echo usage: $0 "{ gcc | clang | debug }"
	exit -1
fi

echo $BUILD_CMD
$BUILD_CMD
