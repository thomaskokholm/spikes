#include "service.hpp"
#include <functional>
#include "../uwsgi/base.hpp"
#include <chrono>

namespace fuwu {
    using namespace std;
    using namespace uWsgi;

    typedef function<void( Session &sess )> session_fn_t;

    /**
        Handle a uwsgi request by checking for existing cookeies and maintaining
        these as sessions. All standart exceptions will be handled here with an attempt to
        report these as nicely as possible (5xx errors)

        Things like client preferred language are handled here to.

        This class does not touch the request body in any way, it may set a new cookie
        header for the response, but the body of the request remain un touched.
    */
    class Handler {
        string _app_name;
        string _cookie_name;
        string _cookie_path;
        int _cookie_age; // in minuts
        int _cookie_min_age;
    public:
        Handler( const string &app_name );

    protected:
        string random_string_create( int size = 20 ) const;
        string cookie_date( chrono::system_clock::time_point tp ) const;
        void cookie_set( Request &req, sess_hndl csess ) const;
        void request( Request &req ) const;

        virtual void handle( Request &req, Session &sess ) const = 0;
    };
}