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
#include "../value/value.hpp"

using namespace std;
using namespace core;

namespace helper {
    template <int... Is>
    struct index {};

    template <int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

    template <int... Is>
    struct gen_seq<0, Is...> : index<Is...> {};
}

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
void for_each_in_tuple(std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>){
    using expander = int[];
    (void)expander { 0, ((void)func(std::get<Is>(tuple)), 0)... };
}

template<class F, class...Ts>
void for_each_in_tuple(std::tuple<Ts...> & tuple, F func){
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

    template <int... Is>
    Ret func(std::tuple<Args...>& tup, helper::index<Is...>) const {
        return _fn(std::get<Is>(tup)...);
    }

    Ret func(std::tuple<Args...>& tup) const {
        return func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

    // make sure to be able to handle void return transparently
    template<typename R = Ret>
    typename enable_if<!is_void<R>::value, Value>::type
    call_fn( std::tuple<Args...>& args ) const {
        return Value( func( args ) );
    }

    template<typename R = Ret>
    typename enable_if<is_void<R>::value, Value>::type
    call_fn( std::tuple<Args...>& args ) const {
        func( args );
        return Value();
    }

    template<typename T> void dump(T t) {
        cout << typeid( t ).name() << endl;
    }

    template<typename T, typename... LArgs> void dump( T t, LArgs... args ) {
        cout << typeid( t ).name() << endl;
        dump( args... );
    }

    // from section 28.6.4 page 817
    // Convert parameters to the proper native type
    template <size_t N>
    struct params_to_tuple {
        template<typename ...T>
        static typename enable_if<(N<sizeof...(T))>::type
        append( tuple<T...>& t, const cvector &params ) {
            if( params.size() <= N ) {
                clog << "more arguments are required " << tuple_size<typename decay<decltype(t)>::type>::value
                    << " than given " << params.size() << endl;
                return;
            }

            if( params[ N ].is_type(get<N>(t)))
                get<N>(t) = params[ N ].get<typename decay<decltype(get<N>(t))>::type>();
            else
                clog << "type error in param " << N << " require " << typeid(get<N>(t)).name() <<
                    " but got " << params[ N ].type_info_get().name() << endl;

            params_to_tuple<N+1>::append(t,params);
        }

        template<typename ...T>
        static typename enable_if<!(N<sizeof...(T))>::type
        append( tuple<T...>& t, const cvector &params ) {
        }
    };

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


    Value call( const cvector &params ) const {
        tuple<Args...> args;

        params_to_tuple<0>::append( args, params ); // Map value types to C++ types

        return call_fn<Ret>( args );
    }
};

// C++ can't infer in template classes !!!
template<typename Ret, typename... Args>
    RpcFunction<Ret, Args...> make_RpcFunction( const function<Ret (Args...)> fn ) {
        return RpcFunction<Ret,Args...>( fn );
    }

struct XXX {
    string name;
    int age;
};

int main( ) {
    auto empty = make_RpcFunction( FFL( []() -> void {
        cout << "empty" << endl;
    }));

    empty.call({});

    auto f = make_RpcFunction( FFL( []( int n, string s, double x, XXX info, bool b ) -> bool {
        cout << n << endl;

        return true;
    }));

//    f.footprint();
    f.call({Value(42), Value{"hest"}, 3.14 });

    f.call({true, 23, 3.14 });

    auto f2 = make_RpcFunction( function<void(map<string, string>, string, bool)>( []( map<string, string>, const string s, bool b ) -> void {
        cout << s << endl;
    }));

    f2.footprint();

    f2.call({32});
}