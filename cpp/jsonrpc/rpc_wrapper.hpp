#ifndef __RPC_WRAPPER_HPP__
#define __RPC_WRAPPER_HPP__ 1

#include "rpc_handler.hpp"
#include "lamda_helper.hpp"
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <functional>

namespace Service {
    using namespace core;
    using namespace std;

    namespace helper {
        template <int... Is>
        struct index {};

        template <int N, int... Is>
        struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

        template <int... Is>
        struct gen_seq<0, Is...> : index<Is...> {};
    }

    template<typename Ret, typename... Args>
    class RpcWrapperFunction : public Rpc {
        typedef function<Ret (Session &, Args...)> Func;
        Func _fn;

        template <int... Is>
        Ret func(Session &sess, tuple<Args...>& tup, helper::index<Is...>) const {
            return _fn(sess, get<Is>(tup)...);
        }

        Ret func(Session &sess, tuple<Args...>& tup) const {
            return func(sess, tup, helper::gen_seq<sizeof...(Args)>{});
        }

        // make sure to be able to handle void return transparently
        template<typename R = Ret>
        typename enable_if<!is_void<R>::value, Value>::type
        call_fn( Session &sess, tuple<Args...>& args ) const {
            return Value( func( sess, args ) );
        }

        template<typename R = Ret>
        typename enable_if<is_void<R>::value, Value>::type
        call_fn( Session &sess, tuple<Args...>& args ) const {
            func( sess, args );
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
                    {"type", Value( json_type( typeid( get<N>(t) )))}
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

        cmap service_def() const {
            tuple<Args ...> args;

            cvector params;
            tuple_types<0>::append( params, args );

            cmap def {
                {"parameters", Value( params )},
                {"return", Value(json_type( typeid( Ret )))}
            };

            return Value( def );
        }

        Value call( Session &sess, const cvector &params ) const {
            tuple<Args...> args;

            params_to_tuple<0>::append( args, params ); // Map value types to C++ types

            return call_fn<Ret>( sess, args );
        }
    };

    // C++ can't infer in template classes !!!
    template<typename Ret, typename... Args>
        RpcWrapperFunction<Ret, Args...> make_RpcFunction( const function<Ret (Session &, Args...)> fn ) {
            return RpcWrapperFunction<Ret,Args...>( fn );
        }
}

#endif