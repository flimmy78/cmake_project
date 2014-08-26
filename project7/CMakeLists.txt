CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(project7 C)

SET(PROJECT_NAME SIMPLE)

SET(SIMPLE_MAJOR 1)
SET(SIMPLE_MINOR 8)
SET(SIMPLE_PATCH 6)
SET(SIMPLE_VERSION ${SIMPLE_MAJOR}.${SIMPLE_MINOR}.${SIMPLE_PATCH})


CONFIGURE_FILE(config.h.cmake config.h)

ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

ADD_EXECUTABLE(simple main.c)

