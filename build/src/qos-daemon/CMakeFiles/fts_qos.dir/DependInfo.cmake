# The set of languages for which implicit dependencies are needed:
SET(CMAKE_DEPENDS_LANGUAGES
  "CXX"
  )
# The set of files for implicit dependencies of each language:
SET(CMAKE_DEPENDS_CHECK_CXX
  "/root/fts3/src/qos-daemon/QoSServer.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/QoSServer.cpp.o"
  "/root/fts3/src/qos-daemon/context/ArchivingContext.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/context/ArchivingContext.cpp.o"
  "/root/fts3/src/qos-daemon/context/CDMIQosTransitionContext.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/context/CDMIQosTransitionContext.cpp.o"
  "/root/fts3/src/qos-daemon/context/DeletionContext.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/context/DeletionContext.cpp.o"
  "/root/fts3/src/qos-daemon/context/JobContext.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/context/JobContext.cpp.o"
  "/root/fts3/src/qos-daemon/context/StagingContext.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/context/StagingContext.cpp.o"
  "/root/fts3/src/qos-daemon/fetch/CDMIFetchQosTransition.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/fetch/CDMIFetchQosTransition.cpp.o"
  "/root/fts3/src/qos-daemon/fetch/FetchArchiving.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/fetch/FetchArchiving.cpp.o"
  "/root/fts3/src/qos-daemon/fetch/FetchCancelStaging.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/fetch/FetchCancelStaging.cpp.o"
  "/root/fts3/src/qos-daemon/fetch/FetchDeletion.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/fetch/FetchDeletion.cpp.o"
  "/root/fts3/src/qos-daemon/fetch/FetchStaging.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/fetch/FetchStaging.cpp.o"
  "/root/fts3/src/qos-daemon/main.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/main.cpp.o"
  "/root/fts3/src/qos-daemon/task/ArchivingPollTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/ArchivingPollTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/ArchivingTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/ArchivingTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/BringOnlineTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/BringOnlineTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/CDMIPollTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/CDMIPollTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/DeletionTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/DeletionTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/Gfal2Task.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/Gfal2Task.cpp.o"
  "/root/fts3/src/qos-daemon/task/PollTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/PollTask.cpp.o"
  "/root/fts3/src/qos-daemon/task/QoSTransitionTask.cpp" "/root/fts3/build/src/qos-daemon/CMakeFiles/fts_qos.dir/task/QoSTransitionTask.cpp.o"
  )
SET(CMAKE_CXX_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
SET(CMAKE_TARGET_DEFINITIONS
  "OPENSSL_THREADS"
  "SOCKET_CLOSE_ON_EXIT"
  "WITH_IPV6"
  "_FILE_OFFSET_BITS=64"
  "_LARGEFILE64_SOURCE"
  "_REENTRANT"
  )

# Targets to which this target links.
SET(CMAKE_TARGET_LINKED_INFO_FILES
  "/root/fts3/build/src/db/generic/CMakeFiles/fts_db_generic.dir/DependInfo.cmake"
  "/root/fts3/build/src/config/CMakeFiles/fts_config.dir/DependInfo.cmake"
  "/root/fts3/build/src/common/CMakeFiles/fts_common.dir/DependInfo.cmake"
  "/root/fts3/build/src/msg-bus/CMakeFiles/fts_msg_bus.dir/DependInfo.cmake"
  "/root/fts3/build/src/cred/CMakeFiles/fts_proxy.dir/DependInfo.cmake"
  "/root/fts3/build/src/monitoring/CMakeFiles/fts_msg_ifce.dir/DependInfo.cmake"
  )

# The include file search paths:
SET(CMAKE_C_TARGET_INCLUDE_PATH
  "src/common"
  "../src"
  "src"
  "/usr/include/glib-2.0"
  "/usr/lib64/glib-2.0/include"
  "/usr/include/gfal2"
  )
SET(CMAKE_CXX_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_Fortran_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_ASM_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
