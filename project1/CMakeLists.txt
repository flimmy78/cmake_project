CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(project1 C)

ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Winline -Werror)

ADD_EXECUTABLE(simple main.c)
