CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(project4 C)

# Search for gzip program
INCLUDE (FindGZIP.cmake)

SET(pdip_src pdip.c)
SET(pdip_exe pdip)
SET(pdip_man_src pdip_en.1 pdip_fr.1)
SET(pdip_man_gz pdip_en.1.gz pdip_fr.1.gz)

# Compression of the manuals
FOREACH(man ${pdip_man_src})
  ADD_CUSTOM_COMMAND(OUTPUT ${man}.gz
                     COMMAND ${GZIP_TOOL} -c ${man} > ${man}.gz
                     DEPENDS ${man}
                     COMMENT "Building ${man}.gz")
ENDFOREACH(man)

# Compilation options passed to the compiler
ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

# Build of the program
ADD_EXECUTABLE(${pdip_exe} ${pdip_src} ${pdip_man_gz})

# Installation of the program
INSTALL(TARGETS pdip
        DESTINATION "bin"
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Installation of the manuals
INSTALL(FILES pdip_fr.1.gz
        DESTINATION "share/man/fr/man1"
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ RENAME pdip.1.gz)
INSTALL(FILES pdip_en.1.gz
        DESTINATION "share/man/man1"
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ RENAME pdip.1.gz)

# Script to be executed at installation time (kind of post-intallation script) to
# change the right accesses on the installed files
INSTALL(SCRIPT pdip_chown.cmake)
