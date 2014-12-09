cmake_minimum_required(VERSION 2.8)
project(xproxy)
enable_testing()
set(CMAKE_VERBOSE_MAKEFILE 0)

#-------------------------------------------------------------------------------
# Set compilation warnings
#-------------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(warnings "-Wall -Wextra -Werror")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${wranings}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wranings}")

#-------------------------------------------------------------------------------
# Enable C++11 support for GCC and Clang
#-------------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
endif()

#-------------------------------------------------------------------------------
# Fix boost template compilation error for Clang
# =>  https://github.com/Homebrew/homebrew/issues/22083
#-------------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=256")
endif()

#-------------------------------------------------------------------------------
# Configure boost library
#-------------------------------------------------------------------------------
if(WIN32)
    set(BOOST_ROOT "D:/SDTs/MinGW")
    set(Boost_USE_STATIC_LIBS ON)
endif()

#find_package(Boost 1.55.0 COMPONENTS atomic system date_time regex filesystem thread chrono REQUIRED)
find_package(Boost 1.55.0 COMPONENTS system date_time REQUIRED)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost is not found.")
    return()
endif()

#-------------------------------------------------------------------------------
# Configure OpenSSL library
# : do NOT use find_package(OpenSSL) under Windows, it will find OpenSSL dlls
#   installed by other programs, whose path is included in %PATH% variable
#-------------------------------------------------------------------------------
if(WIN32)
    set(OPENSSL_ROOT_DIR "D:/SDTs/MinGW")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib/libssl.a" "${OPENSSL_ROOT_DIR}/lib/libcrypto.a")
    set(OPENSSL_FOUND "TRUE")
else()
    find_package(OpenSSL)
endif()

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL is not found.")
    return()
endif()

#-------------------------------------------------------------------------------
# Configure log4cpp library
#-------------------------------------------------------------------------------
if(WIN32)
    set(LOG4CPP_ROOT_DIR "D:/SDTs/MinGW")
    set(LOG4CPP_INCLUDE_DIR "${LOG4CPP_ROOT_DIR}/include")
    set(LOG4CPP_LIBRARIES "${LOG4CPP_ROOT_DIR}/lib/liblog4cpp.a")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(LOG4CPP_INCLUDE_DIR "/usr/local/include")
    set(LOG4CPP_LIBRARIES "/usr/local/lib/liblog4cpp.dylib")
endif()

#-------------------------------------------------------------------------------
# Configure http-parser submodule
#-------------------------------------------------------------------------------
set(http-parser_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/http-parser)
file(GLOB http-parser_SOURCE ${http-parser_DIR}/http_parser.c)
add_library(http-parser STATIC ${http-parser_SOURCE})

#-------------------------------------------------------------------------------
# Configure include and source directories
#-------------------------------------------------------------------------------
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
aux_source_directory(${SRC_DIR} SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

#-------------------------------------------------------------------------------
# Setup unit test using google test
#-------------------------------------------------------------------------------
set(TEST_EXE "test-${PROJECT_NAME}")
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(GTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/gtest)

add_subdirectory(${GTEST_DIR})
file(GLOB TEST_SRC ${TEST_DIR}/*.cpp)

# we should also add source code here, not only the testing code
# however, we should remove the main.cpp, otherwise it will overwrite the
# main() function defined in gtest_main
aux_source_directory(${SRC_DIR} SRC_FOR_TEST)
list(REMOVE_ITEM SRC_FOR_TEST ${SRC_DIR}/main.cpp)
add_executable(${TEST_EXE} ${TEST_SRC} ${SRC_FOR_TEST})
add_test(${TEST_EXE} ${TEST_EXE}) # first: test name, second: test command

#-------------------------------------------------------------------------------
# Configure headers and libraries for main program and testing program
#-------------------------------------------------------------------------------
message(STATUS "include: " ${INCLUDE_DIR})
message(STATUS "include: " ${Boost_INCLUDE_DIR})
message(STATUS "include: " ${OPENSSL_INCLUDE_DIR})
message(STATUS "include: " ${LOG4CPP_INCLUDE_DIR})
message(STATUS "include: " ${http-parser_DIR})
message(STATUS "include: " ${gtest_SOURCE_DIR}/include)

include_directories(${INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${LOG4CPP_INCLUDE_DIR})
include_directories(${http-parser_DIR})
include_directories(${gtest_SOURCE_DIR}/include)

foreach(lib ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${LOG4CPP_LIBRARIES})
    message(STATUS "linking: " ${lib})
endforeach(lib)

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LOG4CPP_LIBRARIES})
target_link_libraries(${PROJECT_NAME} http-parser)

target_link_libraries(${TEST_EXE} ${Boost_LIBRARIES})
target_link_libraries(${TEST_EXE} ${OPENSSL_LIBRARIES})
target_link_libraries(${TEST_EXE} ${LOG4CPP_LIBRARIES})
target_link_libraries(${TEST_EXE} gtest gtest_main)
target_link_libraries(${TEST_EXE} http-parser)

#-------------------------------------------------------------------------------
# Configure platform specific libraries
#-------------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
    target_link_libraries(${TEST_EXE} ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()


#-------------------------------------------------------------------------------
# Copy configuration file and certificates to output directory
#-------------------------------------------------------------------------------
configure_file(xproxy.conf xproxy.conf COPYONLY)
file(COPY cert/ DESTINATION cert/)
