#include "variant.hpp"

namespace nbpp {
    Variant::Variant( const Variant &var ) : _data( NULL ) {
        if( var._data != NULL ) {
            _data = var._data;
            _data->_ref++;
        }
    }

    Variant::Variant( Variant && other ) : _data( NULL ) {
        this->_data = other._data;
        other._data = NULL;
    }
    
    Variant::~Variant() {
        if( _data )
            if(--_data->_ref == 0)
                delete _data;
    }

    Variant &Variant::operator=( const Variant &var ) {
        if( var._data )
            var._data->_ref++;
            
        if( _data )
            if( --_data->_ref == 0 )
                delete _data;

        _data = var._data;
        return *this;
    }

    Variant &Variant::operator=( Variant &&other ) {
        if( this != &other ) {
            if( _data )
                if( --_data->_ref == 0 )
                    delete _data;

            _data = other._data;
            other._data = NULL;
        }
        return *this;
    }
    
    void Variant::out( ostream &os ) const {
        if( _data )
            _data->out( os );
        else
            os << "null";
    }
}

std::ostream &operator << ( std::ostream &os, const nbpp::cvector &v )
{
    nbpp::cvector::const_iterator i;

    os << "[";
    for( i = v.begin(); i != v.end(); i++ ) {
        if( i != v.begin())
            os << ", ";
            
        os << *i;
    }
    os << "]";
    return os;
}

std::ostream &operator << ( std::ostream &os, const nbpp::cmap &v )
{
    nbpp::cmap::const_iterator i;

    os << "{";
    for( i = v.begin(); i != v.end(); i++ ) {
        if( i != v.begin())
            os << ", ";
            
        os << "" << i->first << ":" << i->second;
    }
    os << "}";
    return os;
}
