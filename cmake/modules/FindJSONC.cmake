# Find json-c

find_library(JSONC_LIBRARIES
    NAMES json-c json
    HINTS /lib /lib64 /usr/lib /usr/lib64 
    DOC "json-c library"
)

find_path(JSONC_INCLUDE_DIRS
     NAMES json.h
    HINTS /usr/include/json
    DOC "json-c headers"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(json-c
    DEFAULT_MSG JSONC_LIBRARIES JSONC_INCLUDE_DIRS
)
mark_as_advanced(JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
