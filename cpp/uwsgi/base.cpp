#include "base.hpp"
#include "uwsgi.h"
#include <string>
#include <map>
#include <iostream>
#include <streambuf>
#include <vector>

using namespace std;
using namespace uWsgi; // rename this, as this colide with the C module

extern struct uwsgi_server uwsgi;

extern "C" {
    struct uwsgi_plugin UWSGI_MODULE_NAME;
}

// socket rdbuf for C++ streams reading from uwsgi socket
class uwsgi_body_rdbuf : public streambuf
{
    public:
        explicit uwsgi_body_rdbuf(struct wsgi_request * );

        ssize_t total() const {return _total;}
    private:
        // overrides base class underflow()
        streambuf::int_type underflow();

        // copy ctor and assignment not implemented;
        // copying not allowed
        uwsgi_body_rdbuf(const uwsgi_body_rdbuf &);
        uwsgi_body_rdbuf &operator= (const uwsgi_body_rdbuf &);
    private:
        struct wsgi_request *_req;
        ssize_t _total;
        const size_t _put_back;
        vector<char> _buffer;
};

uwsgi_body_rdbuf::uwsgi_body_rdbuf(struct wsgi_request *req )
    : _req( req ), _put_back( 1 ), _buffer( 100 + _put_back )
{
    _total = 0;
    char *end = &_buffer.front() + _buffer.size();
    setg(end, end, end);
}

streambuf::int_type uwsgi_body_rdbuf::underflow()
{
    if (gptr() < egptr()) // buffer not exhausted
        return traits_type::to_int_type(*gptr());

    char *base = &_buffer.front();
    char *start = base;

    if (eback() == base) // true when this isn't the first fill
    {
        // Make arrangements for putback characters
        memmove(base, egptr() - _put_back, _put_back);
        start += _put_back;
    }

    // start is now the start of the buffer, proper.
    // Read from req socket in to the provided buffer
    ssize_t n = _req->socket->proto_read_body( _req, start, _buffer.size() - (start - base));
    if (n == -1)
        return traits_type::eof();

    _total += n;

    // Set buffer pointers
    setg(base, start, start + n);

    return traits_type::to_int_type(*gptr());
}

Request::Request( wsgi_request *wsgi_req ) : _wsgi_req( wsgi_req ), _body_stream( NULL ) {}

Request::~Request()
{
    if(_body_stream)
        delete _body_stream;
}

string Request::method() const
{
    return string( _wsgi_req->method, _wsgi_req->method_len );
}

string Request::content_type() const
{
    return string( _wsgi_req->content_type, _wsgi_req->content_type_len);
}

string Request::uri() const
{
    return string( _wsgi_req->uri, _wsgi_req->uri_len );
}

string Request::remote_addr() const
{
    return string( _wsgi_req->remote_addr, _wsgi_req->remote_addr_len );
}

string Request::query_string() const
{
    return string( _wsgi_req->query_string, _wsgi_req->query_string_len );
}

string Request::protocol() const
{
    return string( _wsgi_req->protocol, _wsgi_req->protocol_len );
}

string Request::script_name() const
{
    return string( _wsgi_req->script_name, _wsgi_req->script_name_len );
}

string Request::host() const
{
    return string( _wsgi_req->host, _wsgi_req->host_len );
}

string Request::user_agent() const
{
    return string( _wsgi_req->user_agent, _wsgi_req->user_agent_len );
}

string Request::document_root() const
{
    return string( _wsgi_req->document_root, _wsgi_req->document_root_len );
}

string Request::encoding() const
{
    return string( _wsgi_req->encoding, _wsgi_req->encoding_len );
}

void Request::add( const string &name, const string &val )
{
    uwsgi_response_add_header(_wsgi_req, (char *)name.data(), name.length(),
        (char *)val.data(), val.length());
}

string Request::get( const string &name ) const
{
    uint16_t len = 0;

    char * str = uwsgi_get_var(_wsgi_req, (char *)name.data(), name.length(), &len);

    return string( str, len );
}

void Request::prepare_headers( const string &st )
{
    uwsgi_response_prepare_headers(_wsgi_req, (char *)st.data(), st.length());
}

void Request::add_content_type( const string &mime_type )
{
    uwsgi_response_add_content_type(_wsgi_req, (char *)mime_type.data(), mime_type.length());
}

void Request::write_body( const string &body )
{
    uwsgi_response_write_body_do(_wsgi_req, (char *)body.data(), body.length());
}

istream &Request::read_body() const
{
    if( _body_stream == nullptr ) {
        auto rdbuf = new uwsgi_body_rdbuf( _wsgi_req );

        const_cast<Request *>( this )->_body_stream = new istream( rdbuf );
    }

    return *_body_stream;
}

string Request::cookie_get( const string &name ) const
{
    uint16_t rlen;
    char *res = uwsgi_get_cookie(_wsgi_req, (char *)name.data(), name.length(), &rlen );

    if( res != nullptr ) {
        return string( res, rlen );
    }

    return "";
}

// clog wrapper would also be cool !!!
void uWsgi::log( const string &s )
{
    uwsgi_log((s + "\n").c_str() );
}

static map<string, reqfn_t> apps;
void uWsgi::register_app( const string &name, reqfn_t fn )
{
    apps.insert( make_pair( name, fn ));
}

// Add a handler function for all incoming requests, mutex ?
reqfn_t req_handler;
void uWsgi::register_request( reqfn_t fn ) {
    req_handler = fn;
}

static function<void()> init_handler;
void uWsgi::register_init( function<void()> fn ) {
    init_handler = fn;
}

int uwsgi_cplusplus_init(){
    uWsgi::log("Initializing c++ plugin");

    if( init_handler )
        init_handler();

    return 0;
}

static void uwsgi_cplusplus_apps_init() {
    log("Initializing apps c++ plugin");
}

struct Context {
    string appid;
    reqfn_t req_handler;
};

static int uwsgi_cplusplus_mount_app(char *mountpoint, char *app_name) {
    auto app = apps.find( app_name );

    if( app != apps.end()) {
        // check for maximum number of apps
        if (uwsgi_apps_cnt >= uwsgi.max_apps) {
            uwsgi_log("ERROR: you cannot load more than %d apps in a worker\n", uwsgi.max_apps);
            return -1;
        }

        int id = uwsgi_apps_cnt;

        auto c = new Context{app->first, app->second};

        uwsgi_app *wi = uwsgi_add_app( id, UWSGI_MODULE_NAME.modifier1,
            mountpoint, strlen( mountpoint ), c, c );

        // XXX callable need to be set or the app will not be found on request

        // the loading time is basically 0 in c/c++ so we can hardcode them
        wi->started_at = uwsgi_now();
        wi->startup_time = 0;

        // ensure app is initialized on all workers (even without a master)
        uwsgi_emulate_cow_for_apps(id);

        //clog << "INFO: app_id " << id << " lazy mapped at '" << mountpoint
        //    << "' for appid '" << app_name << "'" << endl;

        return id;
    }

    clog << "ERROR: unknown app " << app_name << ", can't mount" << endl;
    return -1;
}

static int uwsgi_cplusplus_request(wsgi_request *wsgi_req) {
    if (uwsgi_parse_vars(wsgi_req)) {
        log("Invalid request. skip.");
        return -1;
    }

    Request req( wsgi_req );

    // clog << "script " << wsgi_req->script << endl;
    wsgi_req->app_id = uwsgi_get_app_id(wsgi_req, wsgi_req->appid, wsgi_req->appid_len, UWSGI_MODULE_NAME.modifier1);

    // Check for app handling
    if( wsgi_req->app_id >= 0 ) {
        string appid( wsgi_req->appid, wsgi_req->appid_len );

        // clog << "INFO: appid " << appid << " id " << wsgi_req->app_id << endl;

        if( wsgi_req->app_id >= 0 ) {
            struct uwsgi_app *wi = &uwsgi_apps[wsgi_req->app_id];

            Context *ctx = reinterpret_cast<Context *>( wi->interpreter );

            ctx->req_handler( req );

            return UWSGI_OK;
        }
    }

    if( req_handler )
        req_handler( req );

    return UWSGI_OK;
}

/*static void uwsgi_cplusplus_after_request(wsgi_request *wsgi_req) {
    // call log_request(wsgi_req) if you want a standard logline
    uWsgi::log("logging c++ request");
}*/

class Init {
public:
    Init( struct uwsgi_plugin &plugin ) {
        plugin.name = "uwsgi_cpp";
        plugin.modifier1 = 250;
        plugin.init = uwsgi_cplusplus_init;
        plugin.init_apps = uwsgi_cplusplus_apps_init;
        plugin.mount_app = uwsgi_cplusplus_mount_app;
        plugin.request = uwsgi_cplusplus_request;
        // plugin.after_request = uwsgi_cplusplus_after_request;
    }
};

static Init init( UWSGI_MODULE_NAME );