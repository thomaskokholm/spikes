cmake_minimum_required(VERSION 3.0)

PROJECT( cpp_spikes )

FIND_PROGRAM(UWSGI_BIN NAMES uwsgi)

# ADD_SUBDIRECTORY( value )
ADD_SUBDIRECTORY( jsonrpc )
IF (UWSGI_BIN)
    ADD_SUBDIRECTORY( uwsgi )
    ADD_SUBDIRECTORY( fuwu )
ENDIF(UWSGI_BIN)