#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__ 1

#include <map>
#include <string>
#include <typeindex>
#include "../value/value.hpp"

namespace Service {
    using namespace std;
    using namespace core;

    typedef vector<string> strings;

    string json_type( const type_info &t );

    class Domain {
    protected:
        string _name;

    public:
        string name_get() const {return _name;}

        virtual Value get( const string &name ) const = 0;
        virtual void set( const string &name, const Value &v ) = 0;

        Value operator[]( const string &name ) {return get( name );}
    };

    class Session {
    protected:
        string _id;
    public:
        string id_get() const {return _id;}

        virtual Domain *domain() const = 0;

        virtual Value get( const string &name ) const = 0;
        virtual void set( const string &name, const Value &v ) = 0;

        Value operator[]( const string &name ) {return get( name );}
    };
}

#endif