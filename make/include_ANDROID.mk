## Root dir for the NKD, souch as 
NDK=/home/ubuntu/Downloads/android-ndk-r27/

SYSROOT=${NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot
TOOLCHAIN=${NDK}/toolchains/llvm/prebuilt/linux-x86_64
AR=${TOOLCHAIN}/bin/llvm-ar
CC = ${TOOLCHAIN}/bin/clang
AS=${CC}
CXX=${TOOLCHAIN}/bin/clang++
LD=${CC}
RANLIB=${TOOLCHAIN}/bin/llvm-ranlib
STRIP=${TOOLCHAIN}/bin/llvm-strip
LINKER = $(CC)

ANSI_CFLAGS = -ansi
WARNINGS = -Wno-format

CFLAGS   = -std=c99 -O2 $(WARNINGS) --target=aarch64-none-linux-android33 --sysroot=${SYSROOT}
CXXFLAGS = -std=c++11  -O2 -std=c++11 $(WARNINGS) --target=aarch64-none-linux-android33 --sysroot=${SYSROOT}
FCFLAGS  = -module ./
LFLAGS   = --target=aarch64-none-linux-android33
DEFINES  = -DANDROID
INCLUDES = -I${SYSROOT}/usr/include/aarch64-linux-android/
LIBDIRS  = -L${SYSROOT}/usr/lib/aarch64-linux-android/33/
LIBS     = -lc -landroid
