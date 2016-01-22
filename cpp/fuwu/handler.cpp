#include "handler.hpp"
#include <ctime>
#include <iostream>
#include <iomanip>

using namespace fuwu;
using namespace std;
using namespace uWsgi;

Handler::Handler( const string &app_name ) : _app_name( app_name )
{
    _cookie_name = "sessid";
    _cookie_path = "/";
    _cookie_age = 15;
    _cookie_min_age = 5;

    // Make this a uwsgi app to make uwsgi do the routing
    register_app( _app_name,
        bind( &Handler::request, this, placeholders::_1 ));
}

string Handler::random_string_create( int size ) const
{
    string id;

    while( size-- > 0 ) {
        unsigned char ch;
        do {
            ch = rand() % 122;
        } while( !isalnum( ch ));

        id += ch;
    }
    return id;
}

string Handler::cookie_date( chrono::system_clock::time_point tp ) const
{
    char buf[ 100 ];
    struct tm tm;

    time_t t = chrono::system_clock::to_time_t( tp );

    if( strftime(buf, sizeof( buf ), "%a, %d-%b-%Y %H:%M:%S %Z", localtime_r(&t, &tm)) > 0 )
        return buf;

    return "";
}

void Handler::cookie_set( Request &req, sess_hndl csess ) const
{
    stringstream os;

    os << _cookie_name << "=" << csess->id_get() << "; path=" << _cookie_path
        << "; expires=" << cookie_date( chrono::system_clock::now() +
            chrono::minutes( _cookie_age ));

    req.add("Set-Cookie", os.str());
}

void Handler::request( Request &req ) const
{
    // test for cookie
    string sess_id = req.cookie_get( _cookie_name );

    if( sess_id.empty())
        sess_id = random_string_create();

    // get session
    if( !ClientSessionFactory::inst().has( sess_id )) {
        // make new session if no cookie
        sess_hndl csess = ClientSessionFactory::inst().create( sess_id );
        cookie_set( req, csess );
    }

    sess_hndl csess = ClientSessionFactory::inst().find( sess_id );

    try {
        Session sess( csess );

        handle( req, sess );
    } catch( const exception &ex ) {
        log( "ERROR: while handling request");
        log( ex.what());
        req.prepare_headers(500);
        req.set_body(ex.what(), "text/plain");
    }

    // if it need refresh, set it on the request
    if( csess->age() < chrono::minutes( _cookie_min_age ))
        cookie_set( req, csess );
}