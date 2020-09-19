set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(RDK_BUILD_DIR /home/deg/Projects/zodiac_apollo/onemw/build-brcm972554pck-refboard)
set(RDK_TARGET_PLATFORM apollo)

set(CMAKE_SYSROOT ${RDK_BUILD_DIR}/tmp/sysroots/${RDK_TARGET_PLATFORM}-debug)
set(CMAKE_C_COMPILER ${RDK_BUILD_DIR}/tmp/sysroots/x86_64-linux/usr/bin/arm-rdk-linux-gnueabi/arm-rdk-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER ${RDK_BUILD_DIR}/tmp/sysroots/x86_64-linux/usr/bin/arm-rdk-linux-gnueabi/arm-rdk-linux-gnueabi-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
