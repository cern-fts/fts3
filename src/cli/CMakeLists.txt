cmake_minimum_required (VERSION 2.6)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(fts3_cli_source_files ftsC.cpp ftsFileTransferSoapBindingProxy.cpp CliBase.cpp SrvManager.cpp)
set(fts3_cli_libs gsoap++ cgsi_plugin_cpp boost_program_options)

include_directories("${PROJECT_SOURCE_DIR}")

# GSOAP file generation
find_package(gSOAP)

set(generated_files_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(gsoap_typemap "${PROJECT_SOURCE_DIR}/src/gsoap/typemap.dat")
set(fts3_wsdl "${PROJECT_SOURCE_DIR}/src/ws-ifce/fts3-1.0.0.wsdl")
set(gsoap_header_file "${generated_files_dir}/ftsH.h")

set(gsoap_source_files 
    "${generated_files_dir}/ftsC.cpp" 
    "${generated_files_dir}/ftsFileTransferSoapBindingProxy.cpp" 
)

include_directories (${generated_files_dir})

file(MAKE_DIRECTORY ${generated_files_dir})

add_custom_command(	
    OUTPUT  ${gsoap_header_file}
	DEPENDS  ${gsoap_typemap} ${fts3_wsdl} 
	COMMAND ${GSOAP_WSDL2H} ${GSOAP_276_COMPAT_FLAGS} -e -y  -t ${gsoap_typemap}  ${fts3_wsdl}  -o ${gsoap_header_file})


add_custom_command(	
    OUTPUT ${gsoap_source_files}
	DEPENDS ${gsoap_header_file} 
    COMMAND ${GSOAP_SOAPCPP2} -n -w -i -C -x -p fts -d ${generated_files_dir} -I ${GSOAP_IMPORT_DIR} ${gsoap_header_file} )


add_executable(fts3-transfer-submit fts3-transfer-submit.cpp SubmitTransferCli.cpp ${fts3_cli_source_files})
target_link_libraries (fts3-transfer-submit ${fts3_cli_libs})

add_executable(fts3-transfer-status fts3-transfer-status.cpp TransferStatusCli.cpp ${fts3_cli_source_files})
target_link_libraries (fts3-transfer-status ${fts3_cli_libs})

add_executable(fts3-transfer-list fts3-transfer-list.cpp ListTransferCli.cpp ${fts3_cli_source_files})
target_link_libraries (fts3-transfer-list ${fts3_cli_libs})
