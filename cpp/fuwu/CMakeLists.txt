IF (UWSGI_BIN)
    EXECUTE_PROCESS(COMMAND ${UWSGI_BIN} --cflags
                    OUTPUT_VARIABLE UWSGI_CFLAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    ADD_COMPILE_OPTIONS(-std=c++11)
    ADD_DEFINITIONS(-std=c++11 -DUWSGI_MODULE_NAME=fuwu_plugin ${UWSGI_CFLAGS})

    ADD_LIBRARY( fuwu SHARED
        ../uwsgi/base.cpp
        ../value/value.cpp
        ../value/json.cpp
        ../net/url.cpp
        handler.cpp
        jsonrpc.cpp
        restful.cpp
        service.cpp
    )

    SET_TARGET_PROPERTIES( fuwu
        PROPERTIES
        SUFFIX "_plugin.so"
        PREFIX ""
    )
ENDIF(UWSGI_BIN)