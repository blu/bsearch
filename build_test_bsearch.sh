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
	CFLAGS+=(
		-marm
		-mfpu=neon
	)
elif [[ $UNAME_MACHINE == "aarch64" ]] && [[ ${HOSTTYPE:0:3} == "arm" ]]; then
	# for armv8 devices with aarch64 kernels + armv7 userspaces
	CFLAGS+=(
		-marm
		-mfpu=neon
	)
fi

if [[ $1 == "debug" ]]; then
	CFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG)
else
	# enable some optimisations that may or may not be enabled by the global optimisation level of choice in this compiler version
	CFLAGS+=(
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

if [[ $HOSTTYPE == "aarch64" ]] || [[ ${HOSTTYPE:0:3} == "arm" ]]; then
	# some compilers like clang may fail auto-detecting the host armv7/armv8 cpu; collect all part numbers
	VENDOR=`cat /proc/cpuinfo | grep -m 1 "^CPU implementer" | sed s/^[^[:digit:]]*//`
	UARCH=`cat /proc/cpuinfo | grep "^CPU part" | sed s/^[^[:digit:]]*//`

	if [[ $VENDOR == 0x41 ]]; then # ARM Holdings
		# list in order of preference, in case of big.LITTLE (armv7 and armv8 lumped together)
		if   [ `echo $UARCH | grep -c 0xd09` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a73
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd08` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a72
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd07` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a57
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd05` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a55
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd04` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a35
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd03` -ne 0 ]; then
			CFLAGS+=(
				-march=armv8-a
				-mtune=cortex-a53
				-DCACHELINE_SIZE=64
			)
		elif [ `echo $UARCH | grep -c 0xd01` -ne 0 ]; then
			CFLAGS+=(
				-mcpu=cortex-a32
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc0f` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a15
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc0e` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a17
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc0d` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a12
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc09` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a9
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc08` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a8
				-DCACHELINE_SIZE=32
			)
		elif [ `echo $UARCH | grep -c 0xc07` -ne 0 ]; then
			CFLAGS+=(
				-march=armv7-a
				-mtune=cortex-a7
				-DCACHELINE_SIZE=32
			)
		else
			echo WARNING: unsupported uarch by vendor $VENDOR
			# set compiler flags at your discretion here
			CFLAGS+=(
				-DCACHELINE_SIZE=64
			)
		fi
	else
		echo WARNING: unsupported uarch vendor
		# set compiler flags at your discretion here
		CFLAGS+=(
			-DCACHELINE_SIZE=64
		)
	fi
else
	CFLAGS+=(
		-march=native
		-mtune=native
		-DCACHELINE_SIZE=64
	)
fi

if [[ $1 == "gcc" ]]; then
	BUILD_CMD="g++ "$BUILD_COMMON" "${CFLAGS[@]}" -fpermissive"
elif [[ $1 == "clang" ]]; then
	BUILD_CMD="clang++ "$BUILD_COMMON" "${CFLAGS[@]}" -Wno-shift-op-parentheses"
elif [[ $1 == "debug" ]]; then
	BUILD_CMD="clang++ "$BUILD_COMMON" "${CFLAGS[@]}
else
	echo usage: $0 "{ gcc | clang | debug }"
	exit -1
fi

echo $BUILD_CMD
$BUILD_CMD
