CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(hello CXX)                                                                                                               
                                                                                                      

FIND_PATH(QT_INC_DIR
          NAMES QtGui
          PATHS /usr/include
                /usr/include/qt4)

IF(NOT QT_INC_DIR)

   MESSAGE(FATAL_ERROR "Unable to find Qt include files")

ELSE(NOT QT_INC_DIR)

   STRING(REGEX REPLACE "/QtGui" "" QT_INC_DIR "${QT_INC_DIR}")
   MESSAGE("Found Qt include files in " ${QT_INC_DIR})

ENDIF(NOT QT_INC_DIR)


ADD_DEFINITIONS(-g -O2 -fsigned-char -freg-struct-return -Wall -W -Wshadow -Wpointer-arith -Wcast-qual -Winline -Werror)

INCLUDE_DIRECTORIES(${QT_INC_DIR})

ADD_EXECUTABLE(hello hello.cc)

TARGET_LINK_LIBRARIES(hello QtCore QtGui)
