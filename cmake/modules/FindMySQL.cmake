# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.
#  MYSQL_FOUND       - True if MySQL found.

find_path(MYSQL_INCLUDE_DIR mysql.h
  /usr/local/include/mysql
  /usr/include/mysql
)

find_library(MYSQL_LIBRARY
  NAMES mysqlclient
  PATHS /usr/local/lib /usr/lib
  PATH_SUFFIXES mysql
)

set(MYSQL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set MYSQL_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_INCLUDE_DIRS)
mark_as_advanced(MYSQL_LIBRARIES MYSQL_INCLUDE_DIRS)
