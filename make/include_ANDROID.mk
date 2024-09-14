NDK=PATH_TO_NDK
TOOLCHAIN=${NDK}/toolchains/llvm/prebuilt/linux-x86_64
SYSROOT=${NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot
TARGET=aarch64-none-linux-android33
AR=$TOOLCHAIN/bin/llvm-ar
CC=${TOOLCHAIN}/bin/clang
AS=${CC}
CXX=${TOOLCHAIN}/bin/clang++
LD=${TOOLCHAIN}/bin/ld
RANLIB=${TOOLCHAIN}/bin/llvm-ranlib
STRIP=${TOOLCHAIN}/bin/llvm-strip
LINKER = $(CC)

ANSI_CFLAGS = -ansi
WARNINGS = -Wno-format

CFLAGS   = -std=c99 -O2 $(WARNINGS) --target=${TARGET} --sysroot=${SYSROOT} -fPIC -shared
CXXFLAGS = -std=c++11  -O2 -std=c++11 $(WARNINGS) --target=${TARGET} --sysroot=${SYSROOT} -fPIC -shared
FCFLAGS  = -module ./
LFLAGS   =
DEFINES  = -D_GNU_SOURCE -DANDROID
INCLUDES = -I${SYSROOT}/usr/include/aarch64-linux-android/
LIBDIRS  = -L${SYSROOT}/usr/lib/aarch64-linux-android/33/
LIBS     = -lc -landroid -lm -ldl
