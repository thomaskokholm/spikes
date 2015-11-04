/**
  variadic template for function introspection and calling.

  The goal is to be able to register a c++ function and the
  at compile time extract all info on arguments and return
  value in order to ba able to both ask for this and pack arguments
  for later function calls.

  All in order to make as beautiful a JsonRPC API as possible, in C++, where
  static typing remains but the runtime will be able to create a system
  call using info given at compile time.

  We use C++14 now !!!
*/

#include <functional>
#include <iostream>
#include <tuple>
#include <map>
#include <typeinfo>
#include <typeindex>

using namespace std;

// http://stackoverflow.com/questions/13358672/how-to-convert-a-lambda-to-an-stdfunction-using-templates
// FFL convert lambda to a std::function
template<typename T>
struct memfun_type
{
    using type = void;
};

template<typename Ret, typename Class, typename... Args>
struct memfun_type<Ret(Class::*)(Args...) const>
{
    using type = std::function<Ret(Args...)>;
};

template<typename F>
typename memfun_type<decltype(&F::operator())>::type
FFL(F const &func)
{ // Function from lambda !
    return func;
}

// http://stackoverflow.com/questions/26902633/how-to-iterate-over-a-tuple-in-c-11
template<class F, class...Ts, std::size_t...Is>
void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>){
    using expander = int[];
    (void)expander { 0, ((void)func(std::get<Is>(tuple)), 0)... };
}

template<class F, class...Ts>
void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func){
    for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}

map<type_index, string> json_types {
    {typeid( int ), "number"},
    {typeid( float ), "number"},
    {typeid( double ), "number"},
    {typeid( string ), "string"},
    {typeid( bool ), "boolean"},
    {typeid( void ), "undefined"}
};

template<typename Ret, typename... Args>
class RpcFunction {
    typedef function<Ret (Args...)> Func;
    Func _fn;

    template<typename T> void dump(T t) {
        cout << __PRETTY_FUNCTION__ << endl;
        cout << typeid( t ).name() << endl;
    }

    template<typename T, typename... LArgs> void dump( T t, LArgs... args ) {
        cout << __PRETTY_FUNCTION__ << endl;
        cout << typeid( t ).name() << endl;
        dump( args... );
    }
public:
    RpcFunction( const Func fn ) : _fn( fn ) {
    }

    void footprint() {
        tuple<Args...> args;

        for_each_in_tuple(args, [](auto &x) {
            cout << typeid( x ).name() << " or in JSON " << json_types[ typeid( x )] << endl;
        });

        cout << "Return type " << typeid( Ret ).name() << " JSON " << json_types[ typeid( Ret )] << endl;
    }
};

// C++ can't inferre in template classes !!!
template<typename Ret, typename... Args>
    RpcFunction<Ret, Args...> make_RpcFunction( const function<Ret (Args...)> fn ) {
        return RpcFunction<Ret,Args...>( fn );
    }

struct XXX {
    string name;
    int age;
};

int main( ) {
    auto f = make_RpcFunction( FFL( []( int n, const string s, double x, XXX info, bool b ) -> void {
        cout << n << endl;
    }));

    f.footprint();

    auto f2 = make_RpcFunction( function<void(map<string, string>, string, bool)>( []( map<string, string>, const string s, bool b ) -> void {
        cout << s << endl;
    }));

    f2.footprint();
}