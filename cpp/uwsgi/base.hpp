#include <string>
#include <functional>

struct wsgi_request;

#ifndef UWSGI_MODULE_NAME
#define UWSGI_MODULE_NAME test_hello_plugin
#endif

namespace uWsgi {
    using namespace std;

    class Request {
        wsgi_request *_wsgi_req;
    public:
        Request( struct wsgi_request *wsgi_req );

        void add( const string &name, const string &val );

        string get( const string &name ) const;

        void prepare_headers( const string &st = "200 OK" );
        void add_content_type( const string &mime_type );
        void write_body( const string &body );

        string cookie_get( const string &name ) const;
    };

    typedef function<void(Request &)> reqfn_t;

    void register_init( function<void()> fn );
    void register_app( const string &path, reqfn_t fn );
    void register_request( reqfn_t fn );

    void log( const string &s );

    // register_signal
    // reqster rpc
}