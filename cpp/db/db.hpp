/**
    database handler, using the value types
*/
#include <string>
#include "../value/value.hpp"

namespace db {
    using namespace std;
    using namespace core;

    class Result;
    class Prepare;

    class Connection {
    public:
        void commit();
        void rollback();

        Result query( const string &stmt );
        long update( const string &stmt );

        Prepare prepare( const string &stmt );

        string escape( const Value &v ) const;
        template<typename ...Args> string format( const string &stmt, Args ...args );
    };

    class Prepared {
    public:
        Result query( const cvector &args );
        long update( const cvector &args );
    };

    class Result {
    public:
        bool next();

        Value get( const string &name ) const;
        bool is_null( const string &name ) const;
    };

    class Savepoint {
    public:
        Savepoint( Connection &conn, const string &name );
        ~Savepoint();

        void release();
    };
}