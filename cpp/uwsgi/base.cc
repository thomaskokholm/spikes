#include <uwsgi.h>
#include <string>

using namespace std;

extern struct uwsgi_server uwsgi;

class Request {
    struct wsgi_request *_wsgi_req;
public:
    Request( struct wsgi_request *wsgi_req ) : _wsgi_req( wsgi_req ) {}

    void add( const string &name, const string &val ) {
        uwsgi_response_add_header(_wsgi_req, (char *)name.data(), name.length(),
            (char *)val.data(), val.length());
    }

    string get( const string &name ) {
        uint16_t len = 0;

        char * str = uwsgi_get_var(_wsgi_req, (char *)name.data(), name.length(), &len);

        return string( str, len );
    }

    void prepare_headers( const string &st = "200 OK" ) {
        uwsgi_response_prepare_headers(_wsgi_req, (char *)st.data(), st.length());
    }

    void add_content_type( const string &mime_type ) {
        uwsgi_response_add_content_type(_wsgi_req, (char *)mime_type.data(), mime_type.length());
    }

    void write_body( const string &body ) {
       uwsgi_response_write_body_do(_wsgi_req, (char *)body.data(), body.length());
    }
};

class FakeClass {
public:
    void hello_world(Request &req);
};

void FakeClass::hello_world(Request &req) {
    string path_info = req.get("PATH_INFO");

    req.prepare_headers("200 OK");
    req.add_content_type("text/html");
    req.write_body(path_info);
}

extern "C" int uwsgi_cplusplus_init(){
    uwsgi_log("Initializing example c++ plugin\n");
    return 0;
}

extern "C" int uwsgi_cplusplus_request(struct wsgi_request *wsgi_req) {
     // get uwsgi variables
    if (uwsgi_parse_vars(wsgi_req)) {
        uwsgi_log("Invalid request. skip.\n");
    } else {
        Request req( wsgi_req );

        FakeClass fc;

        fc.hello_world(req);
    }
    return UWSGI_OK;
}

extern "C" void uwsgi_cplusplus_after_request(struct wsgi_request *wsgi_req) {
    // call log_request(wsgi_req) if you want a standard logline
    uwsgi_log("logging c++ request\n");
}