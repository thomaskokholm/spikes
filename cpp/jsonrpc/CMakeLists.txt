SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11" )

ADD_EXECUTABLE( rpc_test rpc_handler.cpp restful_handler.cpp service.cpp ../value/value.cpp testing.cpp )