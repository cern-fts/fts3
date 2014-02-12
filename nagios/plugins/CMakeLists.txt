cmake_minimum_required(VERSION 2.8)

file (GLOB fts_probes check_fts_*)

install (FILES ${fts_probes}
         DESTINATION usr/lib${LIB_SUFFIX}/nagios/plugins/fts/
         PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                     GROUP_EXECUTE GROUP_READ
                     WORLD_EXECUTE WORLD_READ
)

