CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(roof C)

INCLUDE_DIRECTORIES(include lib)

ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(client)
ADD_SUBDIRECTORY(man)
ADD_SUBDIRECTORY(include)
