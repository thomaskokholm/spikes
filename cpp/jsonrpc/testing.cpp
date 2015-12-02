/**
  variadic template for function introspection and calling.

  The goal is to be able to register a c++ function and the
  at compile time extract all info on arguments and return
  value in order to ba able to both ask for this and pack arguments
  for later function calls.

  All in order to make as beautiful a JsonRPC API as possible, in C++, where
  static typing remains but the runtime will be able to create a system
  call using info given at compile time.
*/

#include "rpc_wrapper.hpp"
#include <iostream>

using namespace std;
using namespace core;
using namespace Service;

struct XXX {
    string name;
    int age;
};

class TmpSession : public Session {
    cmap _vals;
public:
    Domain *domain() const {return nullptr;}
    Value get( const string &name ) const {
        auto i = _vals.find( name );

        if( i != _vals.end())
            return i->second;

        return Value();
    }

    void set( const string &name, const Value &v ) {
        _vals[ name ] = v;
    }
};

int main( ) {
    TmpSession sess;

    clog << Value( cvector{Value( 42 )}) << endl;

    auto empty = make_RpcFunction( FFL( [](Session &sess) -> void {
        cout << "empty" << endl;
    }));

    empty.call(sess, {});

    auto f = make_RpcFunction( FFL( []( Session &sess, int n, string s, double x, XXX info, bool b ) -> bool {
        cout << n << endl;

        return true;
    }));

    cout << f.service_def() << endl;

//    f.footprint();
    f.call(sess, {Value(42), Value{"hest"}, Value( 3.14 )});

    f.call(sess, {Value( true ), Value( 23 ), Value( 3.14 )});

    auto f2 = make_RpcFunction( function<void(Session &, cmap, string, bool)>( []( Session &sess, cmap, const string s, bool b ) -> void {
        cout << s << endl;
    }));

    cout << f2.service_def() << endl;

    f2.call(sess, {Value( 32 )});
}