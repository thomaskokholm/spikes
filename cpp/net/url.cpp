#include <string>
#include <algorithm>    // find
#include <istream>
#include <stdexcept>
#include <string.h>

class URI {
    void parse( const std::string &uri );

    std::string _scheme, _host, _port, _path, _query;
public:
    URI( const std::string &uri ) {parse( uri );}
};

using namespace std;

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
            if( isnum( ch ))
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

static list<pair<string,string>> parse_options( istream &is )
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

void URI::parse(const string &uri)
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
        _port = string(hostEnd, portEnd);
    }

    // path
    if (pathStart != uriEnd)
        _path = string(pathStart, queryStart);

    // query
    if (queryStart != uriEnd)
        _query = string(queryStart, uri.end());
}
