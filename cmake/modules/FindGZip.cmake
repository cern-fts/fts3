find_program(GZIP
    NAMES gzip
    PATHS /bin
	/usr/bin
	/usr/local/bin
)

IF(NOT GZIP)
  MESSAGE(FATAL_ERROR "Unable to find 'gzip' program")
ENDIF(NOT GZIP)
