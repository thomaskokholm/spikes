PROJECT(uwsgi_cpp)

# See https://cmake.org/cmake/help/v3.0/policy/CMP0043.html
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
IF (POLICY CMP0043)
  CMAKE_POLICY(SET CMP0043 NEW)
ENDIF()

ADD_DEFINITIONS(-std=c++11)

FIND_PROGRAM(UWSGI_BIN NAMES uwsgi)
IF (UWSGI_BIN)
    EXECUTE_PROCESS(COMMAND ${UWSGI_BIN} --dot-h
                    OUTPUT_FILE uwsgi.h)

    EXECUTE_PROCESS(COMMAND ${UWSGI_BIN} --cflags
                    OUTPUT_VARIABLE UWSGI_CFLAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    ADD_DEFINITIONS(${UWSGI_CFLAGS})

    ADD_LIBRARY(uwsgi_cpp_test SHARED base.cpp test_handler.cpp)

    SET_TARGET_PROPERTIES(uwsgi_cpp_test
        PROPERTIES
        SUFFIX "_plugin.so"
        PREFIX ""
    )

    INSTALL(TARGETS uwsgi_cpp_test DESTINATION lib/uwsgi/plugins)
ELSE (UWSGI_BIN)
    message(FATAL_ERROR "can't find uwsgi core, please install first")
ENDIF (UWSGI_BIN)