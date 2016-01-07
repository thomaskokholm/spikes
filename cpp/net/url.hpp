#include <list>
#include <string>
#include <memory>

namespace net {
    using namespace std;

    typedef list<pair<string,string>> options_t;

    // Model for performing communication to a URL
    class Connection {
    public:
        Connection();
        virtual ~Connection() {}

        void set( const string &name, const string &value );
        bool has( const string &name ) const;
        string get( const string &name ) const;

        string operator[]( const string &name ) const {return get( name );}

        virtual void set_ostream( ostream &os ) = 0;

        virtual bool perform( istream &is, size_t len ) = 0;
        virtual bool perform( const string &data = "" ) = 0;
    };

    class URL {
        void parse( const string &uri );

        string _scheme, _host, _path, _query;
        int _port;
    public:
        URL( const string &uri );

        string url_get() const;

        string scheme() const {return _scheme;}
        string host() const {return _host;}
        string path() const {return _path;}
        string query() const {return _query;}

        void scheme( const string &scheme) {_scheme = scheme;}
        void host( const string &host ) {_host = host;}
        void port( int port ) {_port = port;}
        void path( const string &path ) {_path = path;}
        void query( const string &query ) {_query = query;}

        options_t options() const;

        shared_ptr<Connection> connect() const;
    };
}