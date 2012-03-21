# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
#
#    http://www.apache.org/licenses/LICENSE-2.0 
#
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License. 

cmake_minimum_required(VERSION 2.6)

add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(db)
add_subdirectory(ws-ifce)
add_subdirectory(cli)
add_subdirectory(server)
add_subdirectory(url-copy)



if(FTS3_COMPILE_WITH_UNITTEST)
	add_subdirectory(unittest)
endif()








