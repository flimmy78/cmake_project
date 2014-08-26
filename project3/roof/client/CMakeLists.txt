SET(roof_client_src main.c)

ADD_EXECUTABLE(main ${roof_client_src})

TARGET_LINK_LIBRARIES(main roof)
SET_TARGET_PROPERTIES(main
                      PROPERTIES OUTPUT_NAME roof)
