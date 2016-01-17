ADD_DEFINITIONS(-std=c++11)

ADD_LIBRARY(fuwu SHARED handler.cpp service.cpp ../uwsgi/base.cpp jsonrpc.cpp )
SET_TARGET_PROPERTIES(fuwu
    PROPERTIES
    SUFFIX "_plugin.so"
    PREFIX ""
)
