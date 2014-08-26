cmake_minimum_required(VERSION 2.4)

PROJECT(udpc C)

SET(PROJECT_NAME UDPC)

# Version number
SET(UDPC_MAJOR 1)
SET(UDPC_MINOR 0)
SET(UDPC_PATCH 0)
SET(UDPC_VERSION ${UDPC_MAJOR}.${UDPC_MINOR}.${UDPC_PATCH})


MESSAGE(STATUS "Building UDPC version ${UDPC_VERSION}")

CONFIGURE_FILE(config.h.cmake config.h)


SET(udpc_src udp_console_u.c)
SET(udpc_exe udpc)

ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

# Build of the program
ADD_EXECUTABLE(${udpc_exe} ${udpc_src})

# Build the module
EXECUTE_PROCESS(COMMAND uname -r
                OUTPUT_VARIABLE os_release
                OUTPUT_STRIP_TRAILING_WHITESPACE)
SET(module_path /lib/modules/${os_release})
SET(module_build_path ${module_path}/build)

ADD_CUSTOM_COMMAND(OUTPUT udpc.ko
                   COMMAND make -C ${module_build_path} M=`pwd`
                   DEPENDS udp_console_k.c
                   COMMENT "Building udpc.ko"
                  )

ADD_CUSTOM_TARGET(kudpc ALL DEPENDS udpc.ko)


# Installation of the module
SET(module_install_path ${module_path}/kernel)
INSTALL(FILES udpc.ko
        DESTINATION ${module_install_path}
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Installation of the program
INSTALL(TARGETS udpc
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

