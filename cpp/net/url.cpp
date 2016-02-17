#include <string>
#include <algorithm>    // find
#include <istream>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <cctype>
#include "url.hpp"

using namespace net;
using namespace std;

/*
static string parse_scheme( istream &is )
{
    string scheme;
    // Look for ://
    char ch = is.get();

    while( is ) {
        if( ch != ':' )
            scheme += ch;
        else {
            if( is.get() == '/' && is.get() == '/' )
                break;
            else
                throw invalid_argument( "bad URI syntax" );
        }

        ch = is.get();
    }

    return scheme;
}

static string parse_host( istream &is )
{
    string host;
    // until : ? # or /
    char ch = is.get();
    while( is ) {
        if( strchr( ":?#", ch ) ) {
            is.unget();
            break;
        }

        host += ch;
        ch = is.get();
    }
    return host;
}

static string parse_port( istream &is )
{
    string port;

    // until /
    char ch = is.get();

    if( is && ch == ':' ) {
        ch = is.get();
        while( is ) {
            if( strchr( "/?#", ch ) ) {
                is.unget();
                break;
            }
            if( isdigit( ch ))
                port += ch;
            else
                throw invalid_argument( "port must be a number in URI" );
        }
    }

    return port;
}

static string parse_path( istream &is )
{
    string path;

    // until ? og #
    char ch = is.get();

    while( is ) {
        if( strchr( "?#", ch )) {
            is.unget();
            break;
        }

        path += ch;
        ch = is.get();
    }

    return path;
}
*/
static options_t parse_options( istream &is )
{
    list<pair<string,string>> lst;
    char ch = is.get();

    if( is && ch == '?' ) {
        ch = is.get();
        string n, v;
        bool is_v = false;

        while( is ) {
            if( ch == '&' ) {
                lst.push_back( make_pair( n, v ));
                v = n = "";
                is_v = false;
            } else if( ch == '=' )
                is_v = true;
            else
                (is_v ? v : n ) += ch;

            ch = is.get();
        }
    }

    return lst;
}

options_t URL::options() const
{
    stringstream os( _query );

    return parse_options( os );
}

URL::URL( const string &uri ) : _port( 80 )
{
    parse( uri );
}

void URL::parse(const string &uri)
{
    auto uriEnd = uri.end();

    // get query start
    auto queryStart = find(uri.begin(), uriEnd, L'?');

    // protocol
    auto protocolStart = uri.begin();
    auto protocolEnd = find(protocolStart, uriEnd, L':');            //"://");

    if (protocolEnd != uriEnd) {
        string prot = &*(protocolEnd);
        if ((prot.length() > 3) && (prot.substr(0, 3) == "://")) {
            _scheme = string(protocolStart, protocolEnd);
            protocolEnd += 3;   //      ://
        } else
            protocolEnd = uri.begin();  // no protocol
    } else
        protocolEnd = uri.begin();  // no protocol

    // host
    auto hostStart = protocolEnd;
    auto pathStart = find(hostStart, uriEnd, L'/');  // get pathStart

    auto hostEnd = find(protocolEnd,
        (pathStart != uriEnd) ? pathStart : queryStart,
        L':');  // check for port

    _host = string(hostStart, hostEnd);

    // port
    if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':')) { // we have a port
        hostEnd++;
        auto portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
        _port = stoi(string(hostEnd, portEnd));
    }

    // path
    if (pathStart != uriEnd)
        _path = string(pathStart, queryStart);

    // query
    if (queryStart != uriEnd)
        _query = string(queryStart, uri.end());
}