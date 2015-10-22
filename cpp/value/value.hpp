/**
    Variant template class to handle type strong C++ types in a unified
    template class, that may be used in STL containers and more. This is
    written in C++11 compatible code, and the impl. is move optimized too.

    All values are ref counted, and the class is reentrant but not thread
    safe, as the rest of the std STL lib.

    When using "const char *" type, we automaticly presume the a request
    of a string type, as this will be the case in the most places.

    It may be a good idea to start using std allocator to optimize memory
    usage, but Variant is not a template, only some of its members, soo.

    It is possible to use ostream directly on the Variant as this is
    deligated directly to the contained type.
*/
#ifndef __VARIANT_HPP__
#define __VARIANT_HPP__ 1

#include <string>
#include <ostream>
#include <typeinfo>
#include <stdexcept>

#include <vector>
#include <map>

namespace nbpp {
    using namespace std;

    class Variant {
    public:
        Variant() : _data( NULL ) {}
        
        Variant( const Variant &var );
        Variant( Variant && other );
        
        template <typename T> Variant( T v ) : _data( new Value<T>( v )) {
            _data->_ref++;
        }

        ~Variant();

        Variant &operator=( const Variant &var );
        Variant &operator=( Variant &&other );

        template <typename T> const T & get() const {
            Value<T> *tmp = dynamic_cast<Value<T> *>( _data );
            
            if( tmp == NULL )
                throw invalid_argument( typeid(T).name() + string(" is not a valid type" ));
                
            return tmp->_val;
        }

        template <typename T> operator T () const {
            return get<T>();
        }

        bool is_null() const {return _data == NULL;}

        template <typename T> bool is_type( T v ) const {
            return typeid(*_data) == typeid( v );
        }

        template <typename T> bool is_type() const {
            return typeid(*_data) == typeid(Value<T>);
        }

        void out( ostream &os ) const;
    private:
        class Base {
        public:
            size_t _ref;

            Base() : _ref(0) {}
            virtual ~Base() {}

            virtual void out( ostream &os ) const = 0;
        };

        template <typename T> class Value : public Base {
        public:
            Value( T v ) : _val( v ) {}
            ~Value() {}

            void out( ostream &os ) const { os << _val;}

            T _val;
        };

        Base *_data;
    };

    // specialized for const char * -> string 
    template<> inline Variant::Variant<const char *>( const char *str ) : _data( new Value<string>( str )) {
        _data->_ref++;
    }

    typedef vector<Variant> cvector;
    typedef map<string, Variant> cmap;
}

// Make stream simple to use 
inline std::ostream &operator << ( std::ostream &os, const nbpp::Variant &v ) {v.out( os ); return os;}
std::ostream &operator << ( std::ostream &os, const nbpp::cvector &v );
std::ostream &operator << ( std::ostream &os, const nbpp::cmap &v );
#endif
