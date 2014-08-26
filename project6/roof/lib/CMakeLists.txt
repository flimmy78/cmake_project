# Make shared library libroof.so from 'roof.c'
ADD_LIBRARY(roof SHARED roof.c)

# Set the build version (VERSION) and the API version (SOVERSION)
SET_TARGET_PROPERTIES(roof
                      PROPERTIES
                      VERSION 1.0.0
                      SOVERSION 1)

# Installation of the library
INSTALL(TARGETS roof
        DESTINATION lib
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
