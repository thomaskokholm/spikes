PROJECT(uwsgi_cpp)

# See https://cmake.org/cmake/help/v3.0/policy/CMP0043.html
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
IF (POLICY CMP0043)
  CMAKE_POLICY(SET CMP0043 NEW)
ENDIF()

ADD_DEFINITIONS(-std=c++11)
IF (UWSGI_BIN)
    EXECUTE_PROCESS(COMMAND ${UWSGI_BIN} --dot-h
                    OUTPUT_FILE uwsgi.h)

    EXECUTE_PROCESS(COMMAND ${UWSGI_BIN} --cflags
                    OUTPUT_VARIABLE UWSGI_CFLAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    ADD_DEFINITIONS(${UWSGI_CFLAGS} -DUWSGI_MODULE_NAME=test_hello_plugin)

    ADD_LIBRARY(test_hello SHARED base.cpp test_hello.cpp)
    SET_TARGET_PROPERTIES(test_hello
        PROPERTIES
        SUFFIX "_plugin.so"
        PREFIX ""
    )

#    REMOVE_DEFINITIONS(-DUWSGI_MODULE_NAME=test_hello_plugin)
#    ADD_DEFINITIONS(-DUWSGI_MODULE_NAME=test_threads_plugin)

    ADD_LIBRARY(test_threads SHARED base.cpp test_threads.cpp)
    SET_TARGET_PROPERTIES(test_threads
        PROPERTIES
        SUFFIX "_plugin.so"
        PREFIX ""
    )

    INSTALL(TARGETS test_hello test_threads DESTINATION lib/uwsgi/plugins)
ELSE (UWSGI_BIN)
    message(FATAL_ERROR "can't find uwsgi core, please install first")
ENDIF (UWSGI_BIN)
