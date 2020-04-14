cmake_minimum_required(VERSION 2.8.9)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O0 -ggdb3 -Wall")
set (CMAKE_CXX_FLAGS "-std=c++17 -O0 -ggdb3 -Wall -DBOOST_LOG_DYN_LINK -DBOOST_LOG_USE_NATIVE_SYSLOG")


set (CMAKE_BUILD_TYPE "Debug")


include_directories(${CMAKE_SOURCE_DIR}/ibsendbws)

link_directories(${CMAKE_SOURCE_DIR}/ibsendbws/lib)


set(COMMLIBS uuid numa pthread dl  curl ssl event rdmacm ibverbs event_pthreads)


#add_subdirectory(base)

set (DRPCSERVER_SRC
	 clock.h
	 clock.cpp
	 RdmaServer.h
	 RdmaServer.cpp
    )

add_library(drpcsvrlib OBJECT ${DRPCSERVER_SRC})


add_executable(ibsendbws $<TARGET_OBJECTS:drpcsvrlib> main.cpp)


target_link_libraries(ibsendbws ${COMMLIBS})
