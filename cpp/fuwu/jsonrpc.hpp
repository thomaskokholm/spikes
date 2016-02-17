#ifndef __JSONRPC_HPP__
#define __JSONRPC_HPP__ 1

#include "service.hpp"

namespace fuwu {
    using namespace std;
    using namespace core;

    class Rpc {
    public:
        virtual cmap service_def() const = 0;

        virtual Value call( Session &sess, const cvector &params ) const = 0;
    };

    class RpcDispatcher {
    private:
        map<string, const Rpc *> _methods;

    public:
        void reg( const string &name, const Rpc * );
        void unreg( const string &name );

        bool has_a( const string &name ) const;

        Value call( Session &sess, const string &name, const cvector &params ) const;
    };
}

#endif