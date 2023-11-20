#!/bin/sh
# set -v

DEVICE=linux-lubancat-g++
SCRIPT_PATH=$(pwd)

#源码包名称，5.15版本使用qt-everywhere-opensource-src，其他的是qt-everywhere-src
MAJOR_NAME=qt-everywhere-src
MAJOR_NAME_1=qt-everywhere-opensource-src

#修改需要下载源码版本的前缀和后缀,我们默认使用5.15.8
OPENSRC_VER_PREFIX=5.15
OPENSRC_VER_SUFFIX=.8

#无需修改--自动组合下载版本
OPENSRC_VER=${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}

#修改源码包解压后的名称
PACKAGE_NAME=${MAJOR_NAME}-${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}

#定义编译后安装--生成的文件,文件夹位置路径
INSTALL_PATH_EXT=/opt/${PACKAGE_NAME}/ext
INSTALL_PATH_HOST=/opt/${PACKAGE_NAME}/host

#定义sysroot目录,需根据自己实际放的位置确认，这个默认设置在~/sysroot下
SYSROOT_PATH=~/sysroot

#添加交叉编译工具链路径，根据前面交叉编译器安装的路径设置
CROSS_CHAIN_PREFIX=/opt/gcc-aarch64-linux-gnu-8.3.0/bin/aarch64-linux-gnu-

#定义压缩包名称
COMPRESS_PACKAGE=${MAJOR_NAME}-${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}.tar.xz
COMPRESS_PACKAGE_1=${MAJOR_NAME_1}-${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}.tar.xz

#自动组合下载地址
case ${OPENSRC_VER_PREFIX} in
   5.15)
      DOWNLOAD_LINK=http://download.qt.io/official_releases/qt/${OPENSRC_VER_PREFIX}/${OPENSRC_VER}/single/${COMPRESS_PACKAGE_1}
      ;;
   *)
      DOWNLOAD_LINK=http://download.qt.io/archive/qt/${OPENSRC_VER_PREFIX}/${OPENSRC_VER}/single/${COMPRESS_PACKAGE}
      ;;
esac

#无需修改--自动组合平台路径
CONFIG_PATH=${SCRIPT_PATH}/${PACKAGE_NAME}/qtbase/mkspecs/devices/${DEVICE}

#无需修改--自动组合配置平台路径文件
CONFIG_FILE=${CONFIG_PATH}/qmake.conf

#下载源码包
do_download_src () {
     echo "\033[1;33mstart download ${PACKAGE_NAME}...\033[0m"

     if [ ! -f "${COMPRESS_PACKAGE}" ];then
         if [ ! -d "${PACKAGE_NAME}" ];then
             wget -c ${DOWNLOAD_LINK}
         fi
     fi

     echo "\033[1;33mdone...\033[0m"
}

#解压源码包
do_tar_package () {
     echo "\033[1;33mstart unpacking the ${PACKAGE_NAME} package ...\033[0m"
     if [ ! -d "${PACKAGE_NAME}" ];then
         tar -xf ${COMPRESS_PACKAGE}
     fi
     echo "\033[1;33mdone...\033[0m"
}

#创建和修改配置平台
do_config_before () {
     echo "\033[1;33mstart configure platform...\033[0m"
     cd ${PACKAGE_NAME}

     if [ ! -d "${CONFIG_PATH}" ];then
         cp -a ${SCRIPT_PATH}/${PACKAGE_NAME}/qtbase/mkspecs/devices/linux-generic-g++ ${CONFIG_PATH}
     fi

     echo "#" > ${CONFIG_FILE}
     echo "# qmake configuration for the LubanCat running Linux for debian10 " >> ${CONFIG_FILE}
     echo "#" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "include(../common/linux_device_pre.conf)" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "QMAKE_LIBS_EGL         += -lEGL -lmali" >> ${CONFIG_FILE}
     echo "QMAKE_LIBS_OPENGL_ES2  += -lGLESv2 -lEGL -lmali" >> ${CONFIG_FILE}
     echo "QMAKE_CFLAGS            = -march=armv8-a" >> ${CONFIG_FILE}
     echo "QMAKE_CXXFLAGS          = \$\$QMAKE_CFLAGS" >> ${CONFIG_FILE}
     echo "QMAKE_LFLAGS += -static-libstdc++" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}

     echo "QMAKE_INCDIR_POST += \
         \$\$[QT_SYSROOT]/usr/include \
         \$\$[QT_SYSROOT]/usr/include/\$\${GCC_MACHINE_DUMP} " >> ${CONFIG_FILE}

     echo "QMAKE_LIBDIR_POST += \
         \$\$[QT_SYSROOT]/usr/lib \
         \$\$[QT_SYSROOT]/lib/\$\${GCC_MACHINE_DUMP} \
         \$\$[QT_SYSROOT]/usr/lib/\$\${GCC_MACHINE_DUMP} " >> ${CONFIG_FILE}

     echo "QMAKE_RPATHLINKDIR_POST += \
         \$\$[QT_SYSROOT]/usr/lib \
         \$\$[QT_SYSROOT]/usr/lib/\$\${GCC_MACHINE_DUMP} \
         \$\$[QT_SYSROOT]/lib/\$\${GCC_MACHINE_DUMP} " >> ${CONFIG_FILE}

     echo "" >> ${CONFIG_FILE}
     echo "DISTRO_OPTS += aarch64" >> ${CONFIG_FILE}
     echo "DISTRO_OPTS += deb-multi-arch" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "include(../common/linux_arm_device_post.conf)" >> ${CONFIG_FILE}
     echo "load(qt_config)" >> ${CONFIG_FILE}

     cat ${CONFIG_FILE}
     echo "\033[1;33mdone...\033[0m"
}

#配置选项
do_configure () {
     echo "\033[1;33mstart configure ${PACKAGE_NAME}...\033[0m"
     ./configure \
     -sysroot ${SYSROOT_PATH}  \
     -hostprefix ${INSTALL_PATH_HOST} \
     -extprefix ${INSTALL_PATH_EXT} \
     -device ${DEVICE} \
     -device-option CROSS_COMPILE=${CROSS_CHAIN_PREFIX} \
     -release \
     -opensource \
     -confirm-license \
     -nomake tests  \
     -make libs \
     -opengl es2 \
     -eglfs \
     -xcb \
     -dbus \
     -syslog \
     -sqlite \
     -tslib \
     -fontconfig \
     -pkg-config \
     -skip qtscript \
     -skip qtwebengine  \
     -no-use-gold-linker  \
     -v \
     -recheck-all
     echo "\033[1;33mdone...\033[0m"
}

#编译并且安装
do_make_install () {
     echo "\033[1;33mstart make and install ${PACKAGE_NAME} ...\033[0m"

     if [ ! -d "${CONFIG_PATH}" ];then
         cd ${PACKAGE_NAME}
     fi
     make -j4 && make install
     echo "\033[1;33mdone...\033[0m"
}

#删除下载的文件
do_delete_file () {
     cd ${SCRIPT_PATH}
     if [ -f "${COMPRESS_PACKAGE}" ];then
         sudo rm -f ${COMPRESS_PACKAGE}
     fi
}

# 下载源码包，如果你手动下载，并放在该脚本同级目录下，可以注释掉。
do_download_src

# 解压源码包，如果你手动解压了，并放在该脚本同级目录下，可以注释掉。
do_tar_package

# 配置文件
do_config_before

# 配置qt编译
do_configure

# 编译，安装qt
#do_make_install

# 删除文件
#do_delete_file

exit $?