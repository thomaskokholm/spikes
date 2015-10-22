/**
    Demo of how simple and strong deferred can be expressed in the new
    c++11 standart, using functor, Variadic, inference.

    This is so simple, as we no more need to alot of declarations per
    posible argument sets, because of the Variadic template feature.

    This may be used for things like signal handling and much more.

    This simple implimentation is not thread safe (locking needed) !
*/
#ifndef __DEFERRED_HPP__
#define __DEFERRED_HPP__ 1

#include <functional>
#include <list>
#include <stdexcept>

namespace nbpp {
    using namespace std;

    template<typename Ret, typename... Args> class Deferred {
        typedef std::function<Ret (Args...)> func_t;
        list<func_t> _callee;
        bool _fired;
    public:
        Deferred() : _fired( false ) {}

        void then( function<Ret (Args...)> func ) {
            if( !_fired )
                _callee.push_back( func );
            else
                throw invalid_argument( "Deferred already fired, can't add more actions" );
        }

        Ret call( Args... args ) {
            Ret res;
            
            for( auto fn : _callee )
                res = fn( args... );

            _fired = true;
            return res;
        }
    };
}
#endif
