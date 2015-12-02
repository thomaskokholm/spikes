SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11" )

ADD_EXECUTABLE( rpc_test handler.cpp ../value/value.cpp )