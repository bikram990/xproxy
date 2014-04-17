cmake_minimum_required(VERSION 2.8)
project(xproxy)

enable_testing()

################################################################################
# Set compilation warnings
################################################################################
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(warnings "-Wall -Wextra -Werror")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${wranings}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wranings}")

################################################################################
# Enable C++11 support for GCC and Clang
################################################################################
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif()

################################################################################
# Fix boost template compilation error for Clang
# =>  https://github.com/Homebrew/homebrew/issues/22083
################################################################################
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=256")
endif()

################################################################################
# Configure include and source directories
################################################################################
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${INCLUDE_DIR})
aux_source_directory(${SRC_DIR} SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

################################################################################
# Configure http-parser submodule
################################################################################
set(http-parser_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/http-parser)
file(GLOB http-parser_SOURCE ${http-parser_DIR}/http_parser.c)
include_directories(${http-parser_DIR})
add_library(http-parser STATIC ${http-parser_SOURCE})
target_link_libraries(${PROJECT_NAME} http-parser)

################################################################################
# Configure boost library
################################################################################
if(WIN32)
    set(BOOST_ROOT "D:/SDTs/Boost")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    if(MINGW)
        target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
    elseif(MSVC)
        set(Boost_COMPILER "-vc120")
        add_definitions(-D_WIN32_WINNT=0x0501)
    endif()
endif()

find_package(Boost 1.54.0 COMPONENTS atomic system date_time regex log log_setup filesystem thread chrono REQUIRED)
if(Boost_FOUND)
    if(NOT WIN32)
        add_definitions(-DBOOST_LOG_DYN_LINK)
    endif()
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Boost is not found.")
    return()
endif()

################################################################################
# Configure thread library for unix like systems
################################################################################
if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

################################################################################
# Configure OpenSSL library
################################################################################
if(WIN32)
    set(OPENSSL_ROOT_DIR "D:/SDTs/OpenSSL")
endif()

find_package(OpenSSL)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL is not found.")
    return()
endif()

################################################################################
# Configure log4cpp library
################################################################################
if(WIN32)
    set(LOG4CPP_ROOT_DIR "D:/SDTs/log4cpp")
    set(LOG4CPP_INCLUDE_DIR "${LOG4CPP_ROOT_DIR}/include")
    set(LOG4CPP_LIBRARIES "${LOG4CPP_ROOT_DIR}/lib/log4cpp.lib")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(LOG4CPP_INCLUDE_DIR "/usr/local/include")
    set(LOG4CPP_LIBRARIES "/usr/local/lib/liblog4cpp.dylib")
endif()

include_directories(${LOG4CPP_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${LOG4CPP_LIBRARIES})

################################################################################
# Copy configuration file and certificates to output directory
################################################################################
configure_file(xproxy.conf xproxy.conf COPYONLY)
file(COPY cert/ DESTINATION cert/)

################################################################################
# Setup unit test using google test
################################################################################
set(TEST_EXE "test-${PROJECT_NAME}")
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(GTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/gtest)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # force this option to ON so that Google Test will use /MD instead of /MT
    # /MD is now the default for Visual Studio, so it should be our default, too
    option(gtest_force_shared_crt
           "Use shared CRT even when Google Test is built as static lib."
           ON)
endif()

add_subdirectory(${GTEST_DIR})
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
file(GLOB TEST_SRC ${TEST_DIR}/*.cpp)

add_executable(${TEST_EXE} ${TEST_SRC})
target_link_libraries(${TEST_EXE} gtest gtest_main)

add_test(${TEST_EXE} ${TEST_EXE}) # first: test name, second: test command
