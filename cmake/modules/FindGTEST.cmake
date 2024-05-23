#
# This module detects if GTEST is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
#
# GTEST_LIBRARIES       = full path to the GTEST libraries
# GTEST_SSL_LIBRARIES   = full path to the GTEST ssl libraries
# GTEST_INCLUDE_DIR     = include dir to be used when using the GTEST library
# GTEST_WSDL2H          = wsdl2h binary
# GTEST_SOAPCPP2        = soapcpp2 binary
# GTEST_FOUND           = set to true if GTEST was found successfully
#
# GTEST_LOCATION
#   setting this enables search for GTEST libraries / headers in this location




# -----------------------------------------------------
# GTEST Libraries
# -----------------------------------------------------
find_library(GTEST_LIBRARIES
    NAMES gtest
    HINTS ${GTEST_LOCATION}/lib ${GTEST_LOCATION}/lib64
          ${GTEST_LOCATION}/lib32
    DOC "The main GTEST library"
)

# -----------------------------------------------------
# GTEST Libraries
# -----------------------------------------------------
find_library(GTEST_MAIN_LIBRARIES
    NAMES gtest_main
    HINTS ${GTEST_LOCATION}/lib ${GTEST_LOCATION}/lib64
          ${GTEST_LOCATION}/lib32
    DOC "The main GTEST main library"
)

# -----------------------------------------------------
# GTEST Include Directories
# -----------------------------------------------------
find_path(GTEST_INCLUDE_DIR
    NAMES gtest.h
    HINTS ${GTEST_LOCATION} ${GTEST_LOCATION}/include ${GTEST_LOCATION}/include/gtest ${GTEST_LOCATION}/include/* /usr/include/gtest
    DOC "The GTEST include directory"
)

SET(GTEST_DEFINITIONS "")



# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set GTEST_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTEST DEFAULT_MSG GTEST_LIBRARIES GTEST_MAIN_LIBRARIES
    GTEST_INCLUDE_DIR)
mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARIES )
