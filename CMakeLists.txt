cmake_minimum_required(VERSION 3.10.0)

project(yuexin CXX)

#c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER g++)

option(BUILD_TYPE "debug or release" ON)
if (BUILD_TYPE)
    #debug
    set(CMAKE_BUILD_TYPE Debug)
    message("This is debug edit")
else()
    set(CMAKE_BUILD_TYPE Release)
    #O2优化级别
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")
    message("This is release edit")
endif()

option(UNIT_TEST "whether build unit test" OFF)
if (UNIT_TEST)   
    add_subdirectory(test)
endif()


#使用spdlog库编译 需要定义一个宏
add_definitions(-DSPDLOG_COMPILED_LIB)

aux_source_directory(${PROJECT_SOURCE_DIR}/src  src)

include_directories(/usr/local/include/
                    /home/hanly/lib/boost_1_75_0
                    /usr/local
                    ${PROJECT_SOURCE_DIR}/thirdParty/include/
                    ${PROJECT_SOURCE_DIR}/thirdParty/include/redis
                    ${PROJECT_SOURCE_DIR}/thirdParty/include/mysql
                    ${PROJECT_SOURCE_DIR}/thirdParty/include/spdlog/include
                    ${PROJECT_SOURCE_DIR}/thirdParty/include/webrtc_ns
                    )

link_directories(${PROJECT_SOURCE_DIR}/thirdParty/lib)

add_executable(sip ${src}) 
target_link_libraries(sip
                      spdlog  
                      hiredis 
                      alibabacloud-oss-cpp-sdk 
                      curl 
                      mysqlclient
                      ssl 
                      crypto 
                      event 
                      postproc 
                      digest
                      pthread 
                      dl 
                      z 
                      #bz2
                      ssl_ciph
                      bio)

install(TARGETS sip
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/sip/bin/)
install(DIRECTORY conf 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/sip)
