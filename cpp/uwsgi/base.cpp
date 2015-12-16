#include "base.hpp"
#include <uwsgi.h>
#include <string>
#include <iostream>

using namespace std;
using namespace uwsgi;

extern struct uwsgi_server uswgi_server;

Request::Request( wsgi_request *wsgi_req ) : _wsgi_req( wsgi_req ) {}

void Request::add( const string &name, const string &val ) {
    uwsgi_response_add_header(_wsgi_req, (char *)name.data(), name.length(),
        (char *)val.data(), val.length());
}

string Request::get( const string &name ) const {
    uint16_t len = 0;

    char * str = uwsgi_get_var(_wsgi_req, (char *)name.data(), name.length(), &len);

    return string( str, len );
}

void Request::prepare_headers( const string &st ) {
    uwsgi_response_prepare_headers(_wsgi_req, (char *)st.data(), st.length());
}

void Request::add_content_type( const string &mime_type ) {
    uwsgi_response_add_content_type(_wsgi_req, (char *)mime_type.data(), mime_type.length());
}

void Request::write_body( const string &body ) {
    uwsgi_response_write_body_do(_wsgi_req, (char *)body.data(), body.length());
}

// clog wrapper would also be cool !!!
void uwsgi::log( const string &s )
{
    uwsgi_log((s + "\n").c_str() );
}

// Add a handler function for all incoming requests, mutex ?
uwsgi::reqfn_t req_handler;
void uwsgi::register_request( reqfn_t fn ) {
    req_handler = fn;
}

extern "C" {
    int uwsgi_cplusplus_init(){
        uwsgi::log("Initializing example c++ plugin");
        return 0;
    }

    int uwsgi_cplusplus_request(wsgi_request *wsgi_req) {
        log( __PRETTY_FUNCTION__ );
         // get uwsgi variables
        if (uwsgi_parse_vars(wsgi_req)) {
            uwsgi::log("Invalid request. skip.");
        } else {
            Request req( wsgi_req );

            if( req_handler )
                req_handler( req );
        }
        return UWSGI_OK;
    }

    void uwsgi_cplusplus_after_request(wsgi_request *wsgi_req) {
        // call log_request(wsgi_req) if you want a standard logline
        uwsgi::log("logging c++ request");
    }

    class Init {
    public:
        Init( struct uwsgi_plugin &plugin ) {
            plugin.name = "uwsgi_cpp";
            plugin.modifier1 = 250;
            plugin.init = uwsgi_cplusplus_init;
            plugin.request = uwsgi_cplusplus_request;
            plugin.after_request = uwsgi_cplusplus_after_request;
        }
    };

    struct uwsgi_plugin uwsgi_cpp_test_plugin;
    static Init init( uwsgi_cpp_test_plugin );
}