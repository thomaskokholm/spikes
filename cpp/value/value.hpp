/**
    Value template class to handle type strong C++ types in a unified
    template class, that may be used in STL containers and more. This is
    written in C++11 compatible code, and the impl. is move optimized too.

    All values are ref counted, and the class is reentrant but not thread
    safe, as the rest of the std STL lib.

    When using "const char *" type, we automaticly presume the a request
    of a string type, as this will be the case in the most places.

    It may be a good idea to start using std allocator to optimize memory
    usage, but Value is not a template, only some of its members, soo.

    It is possible to use ostream directly on the Value as this is
    deligated directly to the contained type.

    Special version using shared_ptr instead of ref counter
*/
#ifndef __VALUE_HPP__
#define __VALUE_HPP__ 1

#include <string>
#include <ostream>
#include <sstream>
#include <typeinfo>
#include <stdexcept>
#include <memory>
#include <iostream>

#include <vector>
#include <map>

namespace core {
    using namespace std;

    class Value;

    typedef vector<Value> cvector;
    typedef map<string, Value> cmap;

    class Value {
        void null_error( const string &tname ) const;
    public:
        Value() : _data( NULL ) {}

        Value( const Value &var );
        Value( Value && other );

        Value( const char *str ); // Special version for old style strings

        template <typename T> Value( T v ) : _data( new RealValue<T>( v )) {
            _data = make_shared<RealValue<T>>( v );
        }

        ~Value();

        Value &operator=( const Value &var );
        Value &operator=( Value &&other );

        template <typename T> T & get() {
            if( _data ) {
                auto tmp = dynamic_pointer_cast<RealValue<T>>( _data );

                if( tmp == nullptr )
                    null_error( typeid(T).name());

                return tmp->_val;
            }
            throw invalid_argument( "can't get data from NULL value" );
        }

        template <typename T> const T & get() const {
            if( _data ) {
                auto tmp = dynamic_pointer_cast<const RealValue<T>>( _data );

                if( tmp == nullptr )
                    null_error( typeid(T).name());

                return tmp->_val;
            }
            throw invalid_argument( "can't get data from NULL value" );
        }

        template <typename T> operator T () const {return get<T>();}

        template <typename T> operator T & () {return get<T>();}

        const Value &operator [] ( const string &key ) const {
            const cmap &map = get<cmap>();
            auto i = map.find( key );
            if( i != map.end())
                return i->second;

            throw invalid_argument( key + " is an unknown key" );
        }

        bool is_null() const {return _data == nullptr;}

        template <typename T> bool is_type( T v ) const {
            if( _data )
                return _data->type_info_get() == typeid( v );

            return false;
        }

        template <typename T> bool is_type() const {
            if( _data )
                return _data->type_info_get() == typeid( T );

            return false;
        }

        const type_info &type_info_get() const {
            if( _data )
                return _data->type_info_get();

            return typeid( void );
        }

        string str() const {return get<string>();}

        void out( ostream &os ) const;
    private:
        class Base {
        public:
            virtual ~Base() {}

            virtual const type_info &type_info_get() const = 0;

            virtual void out( ostream &os ) const = 0;
        };

        template <typename T> class RealValue : public Base {
        public:
            RealValue( T v ) : _val( v ) {}
            ~RealValue() {}

            const type_info &type_info_get() const {
                return typeid(_val);
            }

            void out( ostream &os ) const { os << _val;}

            RealValue<T> &operator=( RealValue<T> &o ) {
                _val = o._val;
                return *this;
            }

            T _val;
        };

        shared_ptr<Base> _data;
    };
}

// Make stream simple to use
inline std::ostream &operator << ( std::ostream &os, const core::Value &v ) {v.out( os ); return os;}
std::ostream &operator << ( std::ostream &os, const core::cvector &v );
std::ostream &operator << ( std::ostream &os, const core::cmap &v );
#endif
