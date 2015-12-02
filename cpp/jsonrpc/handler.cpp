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

#include <functional>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include "handler.hpp"
#include <functional>

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
    using type = function<Ret(Args...)>;
};

template<typename F>
typename memfun_type<decltype(&F::operator())>::type
FFL(F const &func)
{ // Function from lambda !
    return func;
}

static map<type_index, string> json_types {
    {typeid( int ), "number"},
    {typeid( float ), "number"},
    {typeid( double ), "number"},
    {typeid( string ), "string"},
    {typeid( bool ), "boolean"},
    {typeid( void ), "undefined"},
    {typeid( cvector ), "array"},
    {typeid( cmap ), "object"}
};

string json_type( const type_info &t )
{
    return json_types[ t ];
}

template<typename Ret, typename... Args>
class RpcWrapperFunction : public RpcFunction {
    typedef function<Ret (Args...)> Func;
    Func _fn;

    template <int... Is>
    Ret func(tuple<Args...>& tup, helper::index<Is...>) const {
        return _fn(get<Is>(tup)...);
    }

    Ret func(tuple<Args...>& tup) const {
        return func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

    // make sure to be able to handle void return transparently
    template<typename R = Ret>
    typename enable_if<!is_void<R>::value, Value>::type
    call_fn( tuple<Args...>& args ) const {
        return Value( func( args ) );
    }

    template<typename R = Ret>
    typename enable_if<is_void<R>::value, Value>::type
    call_fn( tuple<Args...>& args ) const {
        func( args );
        return Value();
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

    template <size_t N>
    struct tuple_types {
        template<typename ...T>
        static typename enable_if<(N<sizeof...(T))>::type
        append( cvector &params, tuple<T...>& t ) {
            params.push_back(Value( cmap{
                {"type", Value( json_types[ typeid( get<N>(t) )] )}
            }));

            tuple_types<N+1>::append(params, t);
        }

        template<typename ...T>
        static typename enable_if<!(N<sizeof...(T))>::type
        append( cvector &params, tuple<T...>& t ) {
        }
    };

public:
    RpcWrapperFunction( const Func fn ) : _fn( fn ) {
    }

    Value service_def() const {
        tuple<Args ...> args;

        cvector params;
        tuple_types<0>::append( params, args );

        cmap def {
            {"parameters", Value( params )},
            {"return", Value(json_types[ typeid( Ret )])}
        };

        return Value( def );
    }

    Value call( const cvector &params ) const {
        tuple<Args...> args;

        params_to_tuple<0>::append( args, params ); // Map value types to C++ types

        return call_fn<Ret>( args );
    }
};

// C++ can't infer in template classes !!!
template<typename Ret, typename... Args>
    RpcWrapperFunction<Ret, Args...> make_RpcFunction( const function<Ret (Args...)> fn ) {
        return RpcWrapperFunction<Ret,Args...>( fn );
    }

struct XXX {
    string name;
    int age;
};

int main( ) {
    clog << Value( cvector{Value( 42 )}) << endl;

    auto empty = make_RpcFunction( FFL( []() -> void {
        cout << "empty" << endl;
    }));

    empty.call({});

    auto f = make_RpcFunction( FFL( []( int n, string s, double x, XXX info, bool b ) -> bool {
        cout << n << endl;

        return true;
    }));

    cout << f.service_def() << endl;

//    f.footprint();
    f.call({Value(42), Value{"hest"}, Value( 3.14 )});

    f.call({Value( true ), Value( 23 ), Value( 3.14 )});

    auto f2 = make_RpcFunction( function<void(cmap, string, bool)>( []( cmap, const string s, bool b ) -> void {
        cout << s << endl;
    }));

    cout << f2.service_def() << endl;

    f2.call({Value( 32 )});
}