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
        istream *_body_stream;
        string _out_body;
    public:
        Request( struct wsgi_request *wsgi_req );
        ~Request();

        bool add( const string &name, const string &val );

        string get( const string &name ) const;

        // Std CGI elements
        string method() const;
        string content_type() const;
        string uri() const;
        string remote_addr() const;
        string query_string() const;
        string protocol() const;
        string script_name() const;
        string host() const;
        string user_agent() const;
        string document_root() const;
        string encoding() const;
        string referer() const;
        string cookie() const;
        string path_info() const;
        string authorization() const;

        bool prepare_headers( int status = 200 );
        bool prepare_headers( const string &st = "200 OK" );
        bool add_content_type( const string &mime_type );
        bool add_content_length( uint64_t length ); // write body does this by it self
        void set_body( const string &body, const string &mime_type = "" );

        istream &read_body() const;

        string cookie_get( const string &name ) const;
    };

    typedef function<void(Request &)> reqfn_t;

    void register_init( function<void()> fn );
    void register_app( const string &name, reqfn_t fn );
    void register_request( reqfn_t fn );

    void log( const string &s );

    // register_signal
    // reqster rpc
}