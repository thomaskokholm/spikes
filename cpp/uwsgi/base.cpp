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

// Helper function for the app handling
class AppHandler {
    reqfn_t _fn;
public:
    AppHandler( reqfn_t fn ) : _fn( fn ) {}

    void request(wsgi_request *wsgi_req) {
        if (uwsgi_parse_vars(wsgi_req)) {
            uWsgi::log("Invalid request. skip.");
        } else {
            Request req( wsgi_req );

            if( _fn )
                _fn( req );
        }
    }

    static void *app_request( wsgi_request *wsgi_req, struct uwsgi_app *app ) {
        auto hndl = static_cast<AppHandler *>( app->interpreter );

        hndl->request( wsgi_req );

        return NULL; // XXX what does this mean
    }
};

static map<string, reqfn_t> apps_queue;
void uWsgi::register_app( const string &path, reqfn_t fn )
{
    if( uwsgi.workers == nullptr ) {
        log( "workers not initealized, prospone adding app" );
        apps_queue.insert( make_pair( path, fn ));
        return;
    }

    // int id = uwsgi.workers[ uwsgi.mywid ].apps_cnt;
    int id = uwsgi_apps_cnt;

    clog << "App id " << id << " added for path '" << path << "'" << endl;

    uwsgi_app *app = uwsgi_add_app( id, UWSGI_MODULE_NAME.modifier1,
        (char *)path.data(), path.length(), new AppHandler( fn ), nullptr );

    app->request_subhandler = AppHandler::app_request;
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

    /*for( auto app: apps_queue ) {
        // int id = uwsgi.workers[ uwsgi.mywid ].apps_cnt;
        int id = uwsgi_apps_cnt;

        clog << "App id " << id << " added for path '" << app.first << "'" << endl;

        uwsgi_app *uapp = uwsgi_add_app( id, UWSGI_MODULE_NAME.modifier1,
            (char *)app.first.data(), app.first.length(), new AppHandler( app.second ), nullptr );

        uapp->request_subhandler = AppHandler::app_request;
    }*/
}

static int uwsgi_cplusplus_mount_app(char *mountpoint, char *app_name) {
    clog << "mountpoint '"  << mountpoint << "' for app " << app_name << endl;

    auto app = apps_queue.find( app_name );

    if( app != apps_queue.end()) {
        int id = uwsgi_apps_cnt;

        string appid = app->first;

        uwsgi_app *wi = uwsgi_add_app( id, UWSGI_MODULE_NAME.modifier1,
            (char *)appid.data(), appid.length(), nullptr, nullptr );

        // the loading time is basically 0 in c/c++ so we can hardcode them
        wi->started_at = uwsgi_now();
        wi->startup_time = 0;

        // ensure app is initialized on all workers (even without a master)
        uwsgi_emulate_cow_for_apps(id);
        clog << "INFO: app_id " << id << " lazy mapped for appid '" << appid << "'" << endl;
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

    // Check for app handling
    wsgi_req->app_id = uwsgi_get_app_id(wsgi_req, wsgi_req->appid, wsgi_req->appid_len,
        UWSGI_MODULE_NAME.modifier1);

    string appid( wsgi_req->appid, wsgi_req->appid_len );
    auto app_iter =apps_queue.find( appid );

    clog << "INFO: appid '" << appid << "'" << endl;

    if( wsgi_req->app_id >= 0 && app_iter != apps_queue.end() ) {
        clog << "INFO: found app " << appid << " processing request" << endl;

        app_iter->second( req );
    } else {
        if( req_handler )
            req_handler( req );
    }
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