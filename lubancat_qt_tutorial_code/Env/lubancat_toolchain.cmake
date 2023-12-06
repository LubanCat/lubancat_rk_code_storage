# 这里设置cmake支持的最小版本
cmake_minimum_required(VERSION 3.18)
include_guard(GLOBAL)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 指定sysroot目录
set(TARGET_SYSROOT /home/llh/sysroot)

# 指定前面获取的交叉编译器路径
set(CROSS_COMPILER /opt/gcc-aarch64-linux-gnu-8.3.0/bin/aarch64-linux-gnu)

set(CMAKE_SYSROOT ${TARGET_SYSROOT})

set(CMAKE_C_COMPILER ${CROSS_COMPILER}-gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILER}-g++)

set(CMAKE_LIBRARY_ARCHITECTURE aarch64-linux-gnu)

# 库路径
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wl,-rpath-link,${CMAKE_SYSROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE} -fPIC -Wl,-rpath-link,${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} -L${CMAKE_SYSROOT}/lib -L${CMAKE_SYSROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}  -L${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} -L${CMAKE_SYSROOT}/usr/lib -I${TARGET_SYSROOT}/usr/include -I${TARGET_SYSROOT}/usr/include/${CMAKE_LIBRARY_ARCHITECTURE}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# pkg config路径
set(ENV{PKG_CONFIG_PATH} ${TARGET_SYSROOT}/usr/lib/pkgconfig:${TARGET_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig:${TARGET_SYSROOT}/usr/share/pkgconfig/)
set(ENV{PKG_CONFIG_LIBDIR} ${TARGET_SYSROOT}/usr/lib/pkgconfig:${TARGET_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig:${TARGET_SYSROOT}/usr/share/pkgconfig/)
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})

set(QT_COMPILER_FLAGS "-march=armv8-a")
set(QT_COMPILER_FLAGS_RELEASE "-O2 -pipe")
set(QT_LINKER_FLAGS "-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,-fuse-ld=gold -ldbus-1")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

include(CMakeInitializeConfigs)

function(cmake_initialize_per_config_variable _PREFIX _DOCSTRING)
if (_PREFIX MATCHES "CMAKE_(C|CXX|ASM)_FLAGS")
   set(CMAKE_${CMAKE_MATCH_1}_FLAGS_INIT "${QT_COMPILER_FLAGS}")

   foreach (config DEBUG RELEASE MINSIZEREL RELWITHDEBINFO)
      if (DEFINED QT_COMPILER_FLAGS_${config})
      set(CMAKE_${CMAKE_MATCH_1}_FLAGS_${config}_INIT "${QT_COMPILER_FLAGS_${config}}")
      endif()
   endforeach()
endif()

if (_PREFIX MATCHES "CMAKE_(SHARED|MODULE|EXE)_LINKER_FLAGS")
   foreach (config SHARED MODULE EXE)
      set(CMAKE_${config}_LINKER_FLAGS_INIT "${QT_LINKER_FLAGS}")
   endforeach()
endif()

_cmake_initialize_per_config_variable(${ARGV})
endfunction()
