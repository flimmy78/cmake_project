# Search for gzip program
INCLUDE (FindGZIP.cmake)

# Lists of source manuals
SET(roof_man_src_1 roof.1)
SET(roof_man_src_3 roof_cwd.3
                   roof_login.3
                   roof_pwd.3
                   roof_syst.3
                   roof_delete.3
                   roof_mkdir.3
                   roof_retr.3
                   roof_type.3
                   roof.3
                   roof_get_debug_level.3
                   roof_mv.3
                   roof_rm.3
                   roof_get_reply.3
                   roof_new.3
                   roof_rmdir.3
                   roof_cdup.3
                   roof_initialize.3
                   roof_nlst.3
                   roof_set_debug_level.3
                   roof_close_ctrl.3
                   roof_list.3
                   roof_open_ctrl.3
                   roof_stor.3)
SET(roof_man_src_7 roof.7)

# Lists of compressed manuals
STRING(REGEX REPLACE ".1" ".1.gz" roof_man_gz_1 "${roof_man_src_1}")
STRING(REGEX REPLACE ".3" ".3.gz" roof_man_gz_3 "${roof_man_src_3}")
STRING(REGEX REPLACE ".7" ".7.gz" roof_man_gz_7 "${roof_man_src_7}")


# Compression of the manuals
FOREACH(man ${roof_man_src_1} ${roof_man_src_3} ${roof_man_src_7})
  ADD_CUSTOM_COMMAND(OUTPUT ${man}.gz
                     COMMAND ${GZIP_TOOL} -c ${man} > ${man}.gz
                     DEPENDS ${man}
                     COMMENT "Building ${man}.gz")
ENDFOREACH(man)

# Add the manual generation in the global rules
ADD_CUSTOM_TARGET(man ALL
                  DEPENDS ${roof_man_gz_1} ${roof_man_gz_3} ${roof_man_gz_7})

# Installation of the manuals
INSTALL(FILES ${roof_man_gz_1}
        DESTINATION "share/man/man1"
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
INSTALL(FILES ${roof_man_gz_3}
        DESTINATION "share/man/man3"
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
INSTALL(FILES ${roof_man_gz_7}
        DESTINATION "share/man/man7"
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
