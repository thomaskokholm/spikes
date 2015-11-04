#include "value.hpp"

using namespace std;

namespace core {
    Value::Value( const char *str ) : _data( new RealValue<string>( str )) {
    }

    void Value::null_error( const string &tname ) const {
            stringstream os;

            os << tname << " is not a valid destination type, for ";

            if( _data )
                os << _data->type_name();
            else
                os << "NULL";

            throw invalid_argument( os.str());
        }

    Value::Value( const Value &var )
    {
        _data = var._data;
    }

    Value::Value( Value && other )
    {
        _data = move(other._data);
    }

    Value::~Value()
    {
    }

    Value &Value::operator=( const Value &var )
    {
        _data = var._data;
        return *this;
    }

    Value &Value::operator=( Value &&other )
    {
        _data = move(other._data);
        return *this;
    }

    void Value::out( ostream &os ) const {
        if( _data )
            _data->out( os );
        else
            os << "null";
    }


}

ostream &operator << ( ostream &os, const core::cvector &v )
{
    os << "[";
    for( auto i = v.begin(); i != v.end(); i++ ) {
        if( i != v.begin())
            os << ", ";

        os << *i;
    }
    os << "]";
    return os;
}

ostream &operator << ( ostream &os, const core::cmap &v )
{
    os << "{";
    for( auto i = v.begin(); i != v.end(); i++ ) {
        if( i != v.begin())
            os << ", ";

        os << "" << i->first << ":" << i->second;
    }
    os << "}";
    return os;
}
