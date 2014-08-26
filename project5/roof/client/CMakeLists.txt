# 'main' depends on some C source files
ADD_EXECUTABLE(main main.c)

# 'main' depends on libroof.so
TARGET_LINK_LIBRARIES(main roof)

# In the preceding rules, we can't use 'roof' as target name otherwise
# cmake will return in error with TARGET_LINK_LIBRARIES(roof roof):
#
#    CMake Error: Attempt to add link target roof of type: EXECUTABLE
#    to target roof. You can only link to STATIC or SHARED libraries.
#
# Hence the SET_TARGET_PROPERTIES to rename main to roof
#
SET_TARGET_PROPERTIES(main
                      PROPERTIES OUTPUT_NAME roof)

# Installation of the program
INSTALL(TARGETS main
        RUNTIME
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
