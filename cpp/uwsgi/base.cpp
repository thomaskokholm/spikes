#include "base.hpp"
#include "uwsgi.h"
#include <string>
#include <map>
#include <iostream>

using namespace std;
using namespace uWsgi; // rename this, as this colide with the C module

extern struct uwsgi_server uwsgi;

extern "C" {
    struct uwsgi_plugin UWSGI_MODULE_NAME;
}

Request::Request( wsgi_request *wsgi_req ) : _wsgi_req( wsgi_req ) {}

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
        // uwsgi_emulate_cow_for_apps(id);

        clog << "INFO: app_id " << id << " lazy mapped at '" << mountpoint
            << "' for appid '" << app_name << "'" << endl;

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

        clog << "INFO: appid " << appid << " id " << wsgi_req->app_id << endl;

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

static void uwsgi_cplusplus_after_request(wsgi_request *wsgi_req) {
    // call log_request(wsgi_req) if you want a standard logline
    uWsgi::log("logging c++ request");
}

class Init {
public:
    Init( struct uwsgi_plugin &plugin ) {
        plugin.name = "uwsgi_cpp";
        plugin.modifier1 = 250;
        plugin.init = uwsgi_cplusplus_init;
        plugin.init_apps = uwsgi_cplusplus_apps_init;
        plugin.mount_app = uwsgi_cplusplus_mount_app;
        plugin.request = uwsgi_cplusplus_request;
        plugin.after_request = uwsgi_cplusplus_after_request;
    }
};

static Init init( UWSGI_MODULE_NAME );