PROJECT( nbpp11 )

SET(CMAKE_CXX_FLAGS "--std=c++0x -Wall --pedantic")

ADD_EXECUTABLE( json_test json_test.cpp json.cpp variant.cpp deferred.cpp )

ADD_EXECUTABLE( queue commandqueue.cpp )
