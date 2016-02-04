# This CMake file tries to find the the RabbitMQ library
# The following variables are set:
#   RABBITMQ_FOUND - System has RabbitMQ client
#   RABBITMQ_LIBRARIES - The RabbitMQ client library
#   RABBITMQ_HEADERS - The RabbitMQ client headers
include(CheckCSourceCompiles)

find_library(RABBITMQ_LIBRARIES NAMES rabbitmq)
find_path(RABBITMQ_HEADERS amqp.h)

if(${RABBITMQ_LIBRARIES} MATCHES "NOTFOUND")
    message(STATUS "RabbitMQ library not found.")
    unset(RABBITMQ_LIBRARIES)
else()
    message(STATUS "Found RabbitMQ library: ${RABBITMQ_LIBRARIES}")
endif()

set(CMAKE_REQUIRED_INCLUDES ${RABBITMQ_HEADERS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (RABBITMQ DEFAULT_MSG
    RABBITMQ_LIBRARIES  RABBITMQ_HEADERS
)
mark_as_advanced(RABBITMQ_HEADERS RABBITMQ_LIBRARIES)
