#include <string>
#include <functional>

struct wsgi_request;

namespace uwsgi {
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
    };

    typedef function<void(Request &)> reqfn_t;

    void register_request( reqfn_t fn );

    void log( const string &s );

    // register_signal
    // reqster rpc
}